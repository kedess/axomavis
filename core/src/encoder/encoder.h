#pragma once

#include "../utils/avcodec.h"
#include "../utils/avpacket.h"
#include "../utils/avframe.h"
#include <vector>

namespace axomavis {
    class Encoder {
        public:
            Encoder(const AVCodecParameters * codec_params, AVPixelFormat pix_fmt);
            ~Encoder();
            Encoder(const Encoder &p) = delete;
            Encoder& operator=(const Encoder&) = delete;
            Encoder(Encoder &&p) = delete;
            Encoder& operator=(Encoder&&) = delete;
            std::vector<AVPacketWrapper> encode_gpu(AVFrame * frame);
            std::vector<AVPacketWrapper> encode_cpu(AVFrame * frame);
            AVBufferRef * get_hw_frames_ref();
        private:
            AVEncodeWrapper * encode_wrapper = nullptr;
            AVBufferRef * hw_device_ctx = nullptr;
            AVBufferRef * hw_frames_ref = nullptr;
    };
}