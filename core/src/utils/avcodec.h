#pragma once

#include <stdexcept>
#include <sstream>

extern "C"{
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
}

namespace axomavis {
    class AVDecodeWrapper final {
        public:
            AVDecodeWrapper(const AVCodec * codec, const AVCodecParameters *codec_params){
                ctx = avcodec_alloc_context3(codec);
                if (ctx == nullptr) {
                    throw std::runtime_error("Failed to create AVCodecContext");
                }
                if (avcodec_parameters_to_context(ctx, codec_params) < 0) {
                    throw std::runtime_error("Unexpected error");
                }
                if (avcodec_open2(ctx, codec, nullptr) < 0) {
                    throw std::runtime_error("Codec initialization error");
                }
            }
            ~AVDecodeWrapper() {
                if(ctx) {
                    avcodec_free_context(&ctx);
                }
            }
            AVDecodeWrapper(const AVDecodeWrapper &p) = delete;
            AVDecodeWrapper& operator=(const AVDecodeWrapper&) = delete;
            AVDecodeWrapper(AVDecodeWrapper &&p) {
                std::swap(ctx, p.ctx);
            }
            AVDecodeWrapper& operator=(AVDecodeWrapper&&) = delete;
            AVCodecContext * get() {
                return ctx;
            }
            AVCodecContext * operator->() {
                return ctx;
            }
        private:
            AVCodecContext * ctx = nullptr;
            
    };
    class AVEncodeWrapper final {
        public:
            AVEncodeWrapper(const AVCodec * codec){
                ctx = avcodec_alloc_context3(codec);
                if (ctx == nullptr) {
                    throw std::runtime_error("Failed to create AVCodecContext");
                }
            }
            ~AVEncodeWrapper() {
                if(ctx) {
                    avcodec_free_context(&ctx);
                }
            }
            AVEncodeWrapper(const AVEncodeWrapper &p) = delete;
            AVEncodeWrapper& operator=(const AVEncodeWrapper&) = delete;
            AVEncodeWrapper(AVEncodeWrapper &&p) {
                std::swap(ctx, p.ctx);
            }
            AVEncodeWrapper& operator=(AVEncodeWrapper&&) = delete;
            AVCodecContext * get() {
                return ctx;
            }
            AVCodecContext * operator->() {
                return ctx;
            }
        private:
            AVCodecContext * ctx = nullptr;
    };
}