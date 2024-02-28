#include "writer.h"

#include <filesystem>
#include <stdexcept>
#include <iostream>

namespace fs = std::filesystem;

extern "C"{
    #include <libavutil/mathematics.h>
}

namespace axomavis {
    Writer::Writer(const AVCodec *codec, const char * filename):
    fmt_out(filename, codec) {
    }
    void Writer::recv_pkt(AVPacketWrapper & pkt) {
        auto packet = pkt.get();
        packet->stream_index = 0;
        packet->pos = -1;
        if (av_interleaved_write_frame(fmt_out.get(), packet) < 0 ){
            throw std::runtime_error("Packet recording error");
        }
    }
}