#include "capture.h"
#include <plog/Log.h>
#include <csignal>
#include <cstring>
#include <sstream>
#include "../utils/avformat.h"
#include "../utils/avpkt.h"
#include "../archive/archive.h"

extern "C"{
    #include <libavformat/avformat.h>
}

extern volatile std::sig_atomic_t signal_num;

axomavis::Capture::Capture(const char * id, const char * url) : id(id), url(url) {
}

void axomavis::Capture::run() noexcept {
    try {
        AVFormatInput fmt_in;
        auto fmt_in_ctx = fmt_in.getContext();
        if (avformat_open_input(&fmt_in_ctx, url, nullptr, nullptr) < 0) {
            /*
            * Обнуляем обязательно указатель, так как в случаи сбоя avformat_open_input,
            * функция очищает данные указателя fmt_in_ctx
            */
            fmt_in.resetContext();
            std::stringstream ss;
            ss << "Unable to connect to the video stream [" << id << "]";
            throw std::runtime_error(ss.str());
        }
        if (avformat_find_stream_info(fmt_in_ctx, nullptr) < 0) {
            std::stringstream ss;
            ss << "Not found info for stream [" << id << "]";
            throw std::runtime_error(ss.str());
        }
        auto codec_params_list = fetch_params(fmt_in);

        axomavis::Archive archive(codec_params_list, id);
        axomavis::AVPkt pkt;
        while (signal_num == -1 && av_read_frame(fmt_in_ctx, pkt.getPacket()) >= 0) {
            archive.send_pkt(pkt, fmt_in);
            av_packet_unref(pkt.getPacket());
        }
    } catch (const std::exception & ex) {
        LOGE << ex.what();
    }
}

std::vector<AVCodecParameters *> axomavis::Capture::fetch_params(axomavis::AVFormatInput & fmt_in) {
    std::vector<AVCodecParameters*> codec_params_list;
    auto fmt_in_ctx = fmt_in.getContext();
    for (size_t idx = 0; idx < fmt_in_ctx->nb_streams; idx++) {
        auto codec_params = fmt_in_ctx->streams[idx]->codecpar;
        auto media_type = codec_params->codec_type;
        if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
            std::stringstream ss;
            ss
            << "Video stream: [" << id << "]"
            << ", width: " << codec_params->width
            << ", height: " << codec_params->height
            << ", codec: " << avcodec_get_name(codec_params->codec_id);
            LOGI << ss.str();
            video_stream_index = static_cast<int>(idx);
            codec_params_list.push_back(codec_params);
        }
        if (media_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
            std::stringstream ss;
            ss
            << "Audio stream: " << id
            << ", codec: " << avcodec_get_name(codec_params->codec_id);
            LOGI << ss.str();
            audio_stream_index = static_cast<int>(idx);
            codec_params_list.push_back(codec_params);
        }
    }
    return codec_params_list;
}