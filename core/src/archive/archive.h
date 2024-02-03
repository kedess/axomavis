#pragma once
#include "../utils/avformat.h"
#include "../utils/avpkt.h"
#include <chrono>

namespace axomavis {
    class Archive {
        public:
            Archive(std::vector<AVCodecParameters*> codec_params_list, const char * id);
            ~Archive();
            void send_pkt(AVPkt & pkt, AVFormatInput & fmt_in);
        private:
            AVFormatOutput * fmt_out = nullptr;
            std::vector<AVCodecParameters*> codec_params_list;
            std::chrono::steady_clock::time_point time_point;
            std::string prefix_path;
    };
}