#include "decoder.h"
#include <iostream>

namespace axomavis {
    Decoder::Decoder(const AVCodec * codec, const AVCodecParameters *codec_params): decodec_wrapper(AVDecodeWrapper(codec, codec_params)) {
        // TODO: Делать установку только когда используем CUDA
        if (strcmp(decodec_wrapper->codec->name, "h264_cuvid") == 0 || strcmp(decodec_wrapper->codec->name, "hevc_cuvid") == 0) {
            if (av_hwdevice_ctx_create(&hw_device_ctx, AVHWDeviceType::AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) != 0) {
                std::stringstream ss;
                ss << "Not set found hw_device_ctx";
                throw std::runtime_error("Not set found hw_device_ctx");
            }
            decodec_wrapper->hw_device_ctx = av_buffer_ref(hw_device_ctx);
        }
    }
    Decoder::~Decoder() {
        avcodec_send_packet(decodec_wrapper.get(), nullptr);
        if (hw_device_ctx) {
            av_buffer_unref(&hw_device_ctx);
        }
        avcodec_close(decodec_wrapper.get());
        
    }
    std::optional<AVFrameWrapper> Decoder::decode_gpu(AVPacketWrapper &packet) {
        AVFrameWrapper frame;
        AVFrameWrapper sw_frame;
        auto ctx = decodec_wrapper.get();
        auto res = avcodec_receive_frame(ctx, frame.get());
        if (res == 0) {
            return std::nullopt;
        }
        auto repeat = true;
        while (repeat) {
            auto res = avcodec_send_packet(ctx, packet.get());
            if (res == 0) {
                repeat = false;
            } else if (res != AVERROR(EAGAIN)) {
                return std::nullopt;
            }
            res = avcodec_receive_frame(ctx, frame.get());
            if (res == 0) {
                if(av_hwframe_transfer_data(sw_frame.get(), frame.get(), 0) == 0) {
                    sw_frame->pts = packet->pts;
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
        auto res = avcodec_receive_frame(decodec_wrapper.get(), frame.get());
        if (res == 0) {
            return std::nullopt;
        }
        auto repeat = true;
        while (repeat) {
            auto res = avcodec_send_packet(decodec_wrapper.get(), packet.get());
            if (res == 0) {
                repeat = false;
            } else if (res != AVERROR(EAGAIN)) {
                return std::nullopt;
            }
            res = avcodec_receive_frame(decodec_wrapper.get(), frame.get());
            if (res == 0) {
                frame->pts = packet->pts;
                return std::make_optional<AVFrameWrapper>(std::move(frame));
            }
            if (res != AVERROR(EAGAIN) && res != AVERROR(EOF)) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
}