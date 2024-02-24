#pragma once

#include <stdexcept>
extern "C"{
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
}

namespace axomavis {
    class AVFrameWrapper final {
        public:
            AVFrameWrapper() {
                ptr = av_frame_alloc();
                if (ptr == nullptr) {
                    throw std::runtime_error("Failed to create AVPacket");
                }
            }
            ~AVFrameWrapper() {
                av_frame_free(&ptr);
            }
            AVFrameWrapper(const AVFrameWrapper &p) = delete;
            AVFrameWrapper& operator=(const AVFrameWrapper&) = delete;
            AVFrameWrapper(AVFrameWrapper &&p) {
                std::swap(ptr, p.ptr);
            }
            AVFrameWrapper& operator=(AVFrameWrapper&&) = delete;
            AVFrame * get() {
                return ptr;
            }
            AVFrame * operator->() {
                return ptr;
            }
        private:
            AVFrame * ptr = nullptr;
    };
}