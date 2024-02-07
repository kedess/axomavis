#pragma once

#include <stdexcept>
extern "C"{
    #include <libavformat/avformat.h>
}

namespace axomavis {
    class AVPacketWrapper {
        public:
            AVPacketWrapper() {
                auto ptr = av_packet_alloc();
                if (ptr == nullptr) {
                    throw std::runtime_error("Failed to create AVPacket");
                }
                pkt = ptr;
            }
            ~AVPacketWrapper() {
                if (pkt) {
                    av_packet_unref(pkt);
                    av_packet_free(&pkt);
                }
            }
            AVPacket * getAVPacket() {
                return pkt;
            }
            AVPacketWrapper(const AVPacketWrapper &p) = delete;
            AVPacketWrapper& operator=(const AVPacketWrapper&) = delete;
            AVPacketWrapper(AVPacketWrapper &&p) = delete;
            AVPacketWrapper& operator=(AVPacketWrapper&&) = delete;
        private:
            AVPacket * pkt = nullptr;
    };
}