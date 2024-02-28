#pragma once

#include <vector>
#include "../utils/avformat.h"
#include "../utils/avpacket.h"

namespace axomavis {
    class Writer {
        public:
            Writer(const AVCodec * codec, const char * filename);
            ~Writer() = default;
            Writer(const Writer &p) = delete;
            Writer& operator=(const Writer&) = delete;
            Writer(Writer &&p) = delete;
            Writer& operator=(Writer&&) = delete;
            void recv_pkt(AVPacketWrapper & pkt);
        private:
            AVFormatOutput2 fmt_out;
    };
}