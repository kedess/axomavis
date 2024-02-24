#pragma once

#include <optional>

#include "../utils/avcodec.h"
#include "../utils/avframe.h"
#include "../utils/avpacket.h"

namespace axomavis {
    class Decoder final {
        public:
            Decoder(const AVCodec * codec, const AVCodecParameters *codec_params);
            ~Decoder();
            Decoder(const Decoder &p) = delete;
            Decoder& operator=(const Decoder&) = delete;
            Decoder(Decoder &&p) = delete;
            Decoder& operator=(Decoder&&) = delete;
            std::optional<AVFrameWrapper> decode_gpu(AVPacketWrapper &packet);
            std::optional<AVFrameWrapper> decode_cpu(AVPacketWrapper &packet);
        private:
            AVDecodeWrapper decodec_wrapper;
            AVBufferRef *hw_device_ctx = nullptr;
    };
}