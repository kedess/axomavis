#include "archive.h"
#include <filesystem>
#include <mutex>
#include <condition_variable>
#include <deque>

const int64_t MAX_DURATION_ARCH_FILE_SECS = 60;

namespace fs = std::filesystem;

extern "C"{
    #include <libavutil/mathematics.h>
}
extern std::mutex queue_video_files_mutex;
extern std::condition_variable queue_video_files_cond;
extern std::deque<std::string> queue_video_files;

axomavis::Archive::Archive(std::vector<const AVCodecParameters*> codec_params_list, const std::string & id) 
: codec_params_list(codec_params_list) {
    std::stringstream path;
    path << "/tmp/axomavis/" << id << "/";
    prefix_path = path.str();
    fs::create_directories(prefix_path);
}
axomavis::Archive::~Archive() {
    if(fmt_out) {
        delete fmt_out;
        std::unique_lock<std::mutex> guard(queue_video_files_mutex);
        queue_video_files.push_back(filename);
        queue_video_files_cond.notify_one();
    }
}

void axomavis::Archive::recv_pkt(AVPacketWrapper & pkt, AVFormatInput & fmt_in) {
    auto packet = pkt.getAVPacket();
    auto is_key = packet->flags & AV_PKT_FLAG_KEY;
    if (is_key == 1 && fmt_out) {
        auto tp = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(tp - time_point).count() > MAX_DURATION_ARCH_FILE_SECS) {
            delete fmt_out;
            fmt_out = nullptr;
            std::unique_lock<std::mutex> guard(queue_video_files_mutex);
            queue_video_files.push_back(filename);
            queue_video_files_cond.notify_one();
        }
    }
    if (!fmt_out) {
        time_point = std::chrono::steady_clock::now();
        const auto tp = std::chrono::system_clock::now();
        std::stringstream path;
        path << prefix_path
                 << std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count() << ".ts";
        filename = path.str();
        fmt_out = new AVFormatOutput(filename.c_str(), codec_params_list);
    }

    auto idx = packet->stream_index;
    auto in_stream = fmt_in.getContext()->streams[idx];
    auto out_stream = fmt_out->getContext()->streams[idx];
    packet->pts = av_rescale_q_rnd(packet->pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF);
    packet->dts = av_rescale_q_rnd(packet->dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF);
    packet->duration = av_rescale_q(packet->duration, in_stream->time_base, out_stream->time_base);
    packet->pos = -1;
    av_interleaved_write_frame(fmt_out->getContext(), packet);
}