#pragma once

#include <stdexcept>
extern "C"{
    #include <libavformat/avformat.h>
}

namespace axomavis {
    class AVPkt {
        public:
            AVPkt() {
                auto ptr = av_packet_alloc();
                if (ptr == nullptr) {
                    throw std::runtime_error("Failed to create AVPacket");
                }
                pkt = ptr;
            }
            ~AVPkt() {
                if (pkt) {
                    av_packet_unref(pkt);
                    av_packet_free(&pkt);
                }
            }
            AVPacket * getPacket() {
                return pkt;
            }
        private:
            AVPacket * pkt = nullptr;
    };
}