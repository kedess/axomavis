#include "decoder.h"
#include <iostream>

namespace axomavis {
    Decoder::Decoder(const AVCodec * codec, const AVCodecParameters *codec_params): decodec_wrapper(AVDecodeWrapper(codec, codec_params)) {
        // TODO: Делать установку только когда используем CUDA
        auto ctx = decodec_wrapper.getContext();
        if (strcmp(ctx->codec->name, "h264_cuvid") == 0 || strcmp(ctx->codec->name, "hevc_cuvid") == 0) {
            if (av_hwdevice_ctx_create(&hw_device_ctx, AVHWDeviceType::AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) != 0) {
                std::stringstream ss;
                ss << "Not set found hw_device_ctx";
                throw std::runtime_error("Not set found hw_device_ctx");
            }
            ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
        }
    }
    Decoder::~Decoder() {
        avcodec_send_packet(decodec_wrapper.getContext(), nullptr);
        if (hw_device_ctx) {
            av_buffer_unref(&hw_device_ctx);
        }
        avcodec_close(decodec_wrapper.getContext());
        
    }
    std::optional<AVFrameWrapper> Decoder::decode_gpu(AVPacketWrapper &packet) {
        AVFrameWrapper frame;
        AVFrameWrapper sw_frame;
        auto ctx = decodec_wrapper.getContext();
        auto av_frame = frame.getAVFrame();
        auto sw_av_frame = sw_frame.getAVFrame();
        auto av_packet = packet.getAVPacket();
        auto res = avcodec_receive_frame(ctx, av_frame);
        if (res == 0) {
            return std::nullopt;
        }
        auto repeat = true;
        while (repeat) {
            auto res = avcodec_send_packet(ctx, av_packet);
            if (res == 0) {
                repeat = false;
            } else if (res != AVERROR(EAGAIN)) {
                return std::nullopt;
            }
            res = avcodec_receive_frame(ctx, av_frame);
            if (res == 0) {
                if(av_hwframe_transfer_data(sw_av_frame, av_frame, 0) == 0) {
                    sw_av_frame->pts = av_packet->pts;
                    return std::make_optional<AVFrameWrapper>(std::move(sw_frame));
                } else {
                    return std::nullopt;
                }
            }
            if (res != AVERROR(EAGAIN) && res != AVERROR(EOF)) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    std::optional<AVFrameWrapper> Decoder::decode_cpu(AVPacketWrapper &packet) {
        AVFrameWrapper frame;
        auto ctx = decodec_wrapper.getContext();
        auto av_frame = frame.getAVFrame();
        auto av_packet = packet.getAVPacket();
        auto res = avcodec_receive_frame(ctx, av_frame);
        if (res == 0) {
            return std::nullopt;
        }
        auto repeat = true;
        while (repeat) {
            auto res = avcodec_send_packet(ctx, av_packet);
            if (res == 0) {
                repeat = false;
            } else if (res != AVERROR(EAGAIN)) {
                return std::nullopt;
            }
            res = avcodec_receive_frame(ctx, av_frame);
            if (res == 0) {
                av_frame->pts = av_packet->pts;
                return std::make_optional<AVFrameWrapper>(std::move(frame));
            }
            if (res != AVERROR(EAGAIN) && res != AVERROR(EOF)) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
}