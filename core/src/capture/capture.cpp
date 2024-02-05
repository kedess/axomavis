#include "capture.h"
#include <plog/Log.h>
#include <csignal>
#include <cstring>
#include <sstream>
#include <thread>
#include "../utils/avformat.h"
#include "../utils/avpkt.h"
#include "../archive/archive.h"

extern "C"{
    #include <libavformat/avformat.h>
}

extern volatile std::sig_atomic_t signal_num;

axomavis::Capture::Capture(const char * id, const char * url) : id(id), url(url) {
    state.reset(new StreamPendingState);
    if (strlen(id) == 0 || strlen(url) == 0){
        state.reset(new StreamErrorState("Stream id and url cannot be empty. Incorrect stream configuration"));
    }
}

axomavis::StreamStateEnum axomavis::Capture::getStateType() const {
    return state->get_type();
}

int check_read_frame(void* obj) {
    auto self = reinterpret_cast<axomavis::Capture*>(obj);
    auto tp = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(tp - self->getTimePoint()).count() > 10){
        return 1;
    }
    return 0;
}

std::chrono::steady_clock::time_point axomavis::Capture::getTimePoint() const {
    return time_point;
}

void axomavis::Capture::updateTimePoint() {
    time_point = std::chrono::steady_clock::now();
}

void axomavis::Capture::run() noexcept {
    while (signal_num == -1 && getStateType() != StreamStateEnum::Error) {
        try {
            LOGI << "Attempting to start a stream [" << id << "]";
            updateTimePoint();
            AVFormatInput fmt_in;
            auto fmt_in_ctx = fmt_in.getContext();
            AVIOInterruptCB icb= {check_read_frame,reinterpret_cast<void*>(this)};
            fmt_in_ctx->interrupt_callback = icb;
            fmt_in_ctx->interrupt_callback.opaque = this;
            if (avformat_open_input(&fmt_in_ctx, url, nullptr, nullptr) < 0) {
                /*
                * Обнуляем обязательно указатель, так как в случаи сбоя avformat_open_input,
                * функция очищает данные указателя fmt_in_ctx
                */
                fmt_in.resetContext();
                std::stringstream ss;
                ss << "Unable to connect to the video stream [" << id << "]";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                throw std::runtime_error(ss.str());
            }
            if (avformat_find_stream_info(fmt_in_ctx, nullptr) < 0) {
                std::stringstream ss;
                ss << "Not found info for stream [" << id << "]";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                throw std::runtime_error(ss.str());
            }
            auto codec_params_list = fetch_params(fmt_in);
            LOGI << "The stream has been successfully launched [" << id << "]";

            axomavis::Archive archive(codec_params_list, id);
            axomavis::AVPkt pkt;
            
            set_running_state();
            while (signal_num == -1 && av_read_frame(fmt_in_ctx, pkt.getPacket()) >= 0) {
                updateTimePoint();
                archive.send_pkt(pkt, fmt_in);
                av_packet_unref(pkt.getPacket());
            }
            set_pending_state();
            LOGW << "Connection to the stream [" << id << "]" << " is lost";
            std::this_thread::sleep_for(std::chrono::seconds(5));
        } catch (const std::exception & ex) {
            LOGE << ex.what();
        }
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
void axomavis::Capture::set_pending_state() {
    state->set_pending_state(state);
}
void axomavis::Capture::set_running_state() {
    state->set_pending_state(state);
}
void axomavis::Capture::set_error_state(const char * description) {
    state->set_error_state(state, description);
}