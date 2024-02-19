#pragma once
#include "../utils/avformat.h"
#include "../utils/avpacket.h"
#include <chrono>

namespace axomavis {
    class Archive {
        public:
            Archive(std::vector<const AVCodecParameters*> codec_params_list, const std::string & id);
            ~Archive();
            Archive(const Archive &p) = delete;
            Archive& operator=(const Archive&) = delete;
            Archive(Archive &&p) = delete;
            Archive& operator=(Archive&&) = delete;
            void recv_pkt(AVPacketWrapper & pkt, AVFormatInput & fmt_in);
        private:
            AVFormatOutput * fmt_out = nullptr;
            std::vector<const AVCodecParameters*> codec_params_list;
            std::chrono::steady_clock::time_point time_point;
            std::string prefix_path;
    };
}