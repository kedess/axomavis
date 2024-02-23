#include "encoder.h"

#include <iostream>
#include <sstream>

namespace axomavis {
    Encoder::Encoder(const AVCodecParameters * codec_params, AVPixelFormat pix_fmt){
        const char * names[] = {"h264_nvenc", "libx264"};
        for(auto name : names) {
            auto codec = avcodec_find_encoder_by_name(name);
            if (codec) {
                encode_wrapper = new AVEncodeWrapper(codec);
                auto ctx = encode_wrapper->get();
                ctx->sample_aspect_ratio = codec_params->sample_aspect_ratio;
                ctx->width = codec_params->width;
                ctx->height = codec_params->height;
                ctx->time_base = av_inv_q(codec_params->framerate);
                ctx->framerate = codec_params->framerate;
                ctx->pix_fmt = pix_fmt;
                // TODO: Делать установку только когда используем CUDA
                if (strcmp(name, "h264_nvenc") == 0) {
                    if (av_hwdevice_ctx_create(&hw_device_ctx, AVHWDeviceType::AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) != 0) {
                        throw std::runtime_error("Not init device_ctx");
                    }
                    hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx);
                    if (!hw_frames_ref) {
                        throw std::runtime_error("Not init device_ctx");
                    }
                    AVHWFramesContext * frames_ctx = nullptr;
                    frames_ctx = reinterpret_cast<AVHWFramesContext *>(hw_frames_ref->data);
                    frames_ctx->format = pix_fmt;
                    frames_ctx->sw_format = AV_PIX_FMT_YUV420P;
                    frames_ctx->width = codec_params->width;
                    frames_ctx->height = codec_params->height;

                    if (av_hwframe_ctx_init(hw_frames_ref) < 0 ) {
                        throw std::runtime_error("Not init device_ctx");
                    }

                    ctx->hw_device_ctx = av_buffer_ref(hw_frames_ref);
                    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
                    if (!ctx->hw_device_ctx || !ctx->hw_frames_ctx) {
                        throw std::runtime_error("Not init device_ctx");
                    }
                    std::pair<const char *, const char *> params[] = {
                        std::make_pair<const char*, const char *>("preset", "llhq"),
                        std::make_pair<const char*, const char *>("zerolatency", "1"),
                        std::make_pair<const char*, const char *>("profile", "main"),
                        std::make_pair<const char*, const char *>("level", "4"),
                        std::make_pair<const char*, const char *>("rc", "vbr_hq")
                    };
                    for (auto pair : params) {
                        av_opt_set(ctx->priv_data, pair.first, pair.second, 0);
                    }
                }
                if (strcmp(name, "libx264") == 0){
                    std::pair<const char *, const char *> params[] = {
                        std::make_pair<const char*, const char *>("preset", "ultrafast"),
                        std::make_pair<const char*, const char *>("tune", "zerolatency"),
                        std::make_pair<const char*, const char *>("profile", "main"),
                        std::make_pair<const char*, const char *>("sc_threshold", "40")
                    };
                    for (auto pair : params) {
                        av_opt_set(ctx->priv_data, pair.first, pair.second, 0);
                    }
                }
                if (avcodec_open2(ctx, codec, nullptr) < 0) {
                    throw std::runtime_error("Codec initialization error");
                }
                break;
            }
        }
        if (!encode_wrapper) {
            throw std::runtime_error("Not found video encoder for H264");
        }
    }
    Encoder::~Encoder() {
        if (encode_wrapper) {
            auto ctx = encode_wrapper->get();
            avcodec_send_frame(ctx, nullptr);
            avcodec_close(ctx);
            if (hw_device_ctx) {
                av_buffer_unref(&hw_device_ctx);
            }
            if (hw_frames_ref) {
                av_buffer_unref(&hw_frames_ref);
            }
            delete encode_wrapper;
        }
    }
    std::vector<AVPacketWrapper> Encoder::encode_gpu(AVFrame * av_frame){
        std::vector<AVPacketWrapper> packets;
        AVFrameWrapper hw_frame;
        av_frame->pict_type = AVPictureType::AV_PICTURE_TYPE_NONE;
        auto ctx = encode_wrapper->get();
        auto ret = avcodec_send_frame(ctx, av_frame);
        if (ret < 0) {
            return packets;
        }
        while (ret >= 0) {
            AVPacketWrapper pkt;
            ret = avcodec_receive_packet(ctx, pkt.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR(EOF)) {
                break;
            }
            if (ret < 0) {
                break;
            }
            if (av_hwframe_transfer_data(hw_frame.get(), av_frame, 0) == 0) {
                packets.emplace_back(std::move(pkt));
            }
        }
        return packets;
    }
    std::vector<AVPacketWrapper> Encoder::encode_cpu(AVFrame * av_frame){
        std::vector<AVPacketWrapper> packets;
        av_frame->pict_type = AVPictureType::AV_PICTURE_TYPE_NONE;
        auto ctx = encode_wrapper->get();
        auto ret = avcodec_send_frame(ctx, av_frame);
        if (ret < 0) {
            return packets;
        }
        while (ret >= 0) {
            AVPacketWrapper pkt;
            ret = avcodec_receive_packet(ctx, pkt.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR(EOF)) {
                break;
            }
            if (ret < 0) {
                break;
            }
            packets.push_back(std::move(pkt));
        }
        return packets;
    }
    AVBufferRef * Encoder::get_hw_frames_ref() {
        return hw_frames_ref;
    }
}