#include "capture.h"
#include <plog/Log.h>
#include <csignal>
#include <cstring>
#include <sstream>
#include <thread>
#include "../utils/avformat.h"
#include "../utils/avpacket.h"
#include "../archive/archive.h"

extern "C"{
    #include <libavformat/avformat.h>
}

extern volatile std::sig_atomic_t signal_num;

size_t axomavis::Capture::numbers = 0;

axomavis::Capture::Capture(axomavis::Source source): source(source) {
    numbers++;
    state.reset(new StreamPendingState);
    if (this->source.getId().size() == 0 || this->source.getUrl().size() == 0){
        state.reset(new StreamErrorState("Stream id and url cannot be empty. Incorrect stream configuration"));
    }
}

axomavis::StreamStateEnum axomavis::Capture::getStateType() const {
    return state->get_type();
}

int check_latency_read_packet(void* obj) {
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
    /*
    * Крутимся в цикле и пытаемя подключится заново, пока мы не в состоянии ERROR
    * или приложение не остановлено по требованию
    */
    while (signal_num == -1 && getStateType() != StreamStateEnum::Error) {
        try {
            LOGI << "Attempting to start a stream [" << source.getId() << "]";
            updateTimePoint();
            AVFormatInput fmt_in;
            /*
            * Функция следит за временем задержки получения пакетов из потока,
            * используя переменную time_point
            */
            AVIOInterruptCB icb= {check_latency_read_packet,reinterpret_cast<void*>(this)};
            fmt_in->interrupt_callback = icb;
            fmt_in->interrupt_callback.opaque = this;
            auto ptr = fmt_in.get();
            if (avformat_open_input(&ptr, source.getUrl().c_str(), nullptr, nullptr) < 0) {
                /*
                * Обнуляем обязательно указатель, так как в случаи сбоя avformat_open_input,
                * функция очищает данные указателя fmt_in
                */
                ptr = nullptr;
                std::stringstream ss;
                ss << "Unable to connect to the video stream [" << source.getId() << "]";
                throw std::runtime_error(ss.str());
            }
            if (avformat_find_stream_info(fmt_in.get(), nullptr) < 0) {
                std::stringstream ss;
                ss << "Not found info for stream [" << source.getId() << "]";
                throw std::runtime_error(ss.str());
            }
            auto params_list = fetch_input_stream_params(fmt_in);
            LOGI << "The stream has been successfully launched [" << source.getId() << "]";

            axomavis::Archive archive(params_list, source.getId().c_str());
            axomavis::AVPacketWrapper packet;
            
            /*
            * Соединение прошло успешно, потоки определены выставляем состояние RUNNING
            */
            set_running_state();
            while (signal_num == -1 && av_read_frame(fmt_in.get(), packet.get()) == 0) {
                updateTimePoint();
                archive.recv_pkt(packet, fmt_in);
                av_packet_unref(packet.get());
            }
            /*
            * Пропало соединен с потоком, ставим состояние PENDING
            */
            if (signal_num == -1) {
                set_pending_state();
                LOGW << "Connection to the stream [" << source.getId() << "]" << " is lost";
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        } catch (const std::exception & ex) {
            LOGE << ex.what();
            /*
            * По какой то причине, но не критической получили исключение ставим состояние PENDING
            * и попытаемся соединится снова
            */
            set_pending_state();
            /*
            * Чтобы не было постоянной попытки подсоединения в логах, после каждой ошибки немного ждем
            */
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

std::vector<const AVCodecParameters *> axomavis::Capture::fetch_input_stream_params(axomavis::AVFormatInput & fmt_in) {
    std::vector<const AVCodecParameters*> params_list;
    for (size_t idx = 0; idx < fmt_in->nb_streams; idx++) {
        auto codec_params = fmt_in->streams[idx]->codecpar;
        auto media_type = codec_params->codec_type;
        if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
            std::stringstream ss;
            ss
            << "Video stream: [" << source.getId() << "]"
            << ", width: " << codec_params->width
            << ", height: " << codec_params->height
            << ", codec: " << avcodec_get_name(codec_params->codec_id);
            LOGI << ss.str();
            switch (codec_params->codec_id) {
                case AVCodecID::AV_CODEC_ID_H264: {
                    const char * names[] = {"h264_cuvid", "h264"};
                    const AVCodec * codec = nullptr;
                    for(auto name : names) {
                        codec = avcodec_find_decoder_by_name(name);
                        if (codec) {
                            params_list.push_back(codec_params);
                            break;
                        }
                    }
                    if (!codec) {
                        std::stringstream ss;
                        ss << "Not found video decoder [" << avcodec_get_name(codec_params->codec_id) << "]";
                        throw std::runtime_error(ss.str());
                    }
                    break;
                }
                case AVCodecID::AV_CODEC_ID_HEVC: {
                    const char * names[] = {"hevc_cuvid", "h265", "hevc"};
                    const AVCodec * codec = nullptr;
                    for(auto name : names) {
                        codec = avcodec_find_decoder_by_name(name);
                        if (codec) {
                            params_list.push_back(codec_params);
                            break;
                        }
                    }
                    if (!codec) {
                        std::stringstream ss;
                        ss << "Not found video decoder [" << avcodec_get_name(codec_params->codec_id) << "]";
                        throw std::runtime_error(ss.str());
                    }
                    break;
                }
                default: {
                    std::stringstream ss;
                    ss << "Unsupported video decoder [" << avcodec_get_name(codec_params->codec_id) << "]";
                    throw std::runtime_error(ss.str());
                }
            }
        }
        if (media_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
            std::stringstream ss;
            ss
            << "Audio stream: " << source.getId()
            << ", codec: " << avcodec_get_name(codec_params->codec_id);
            LOGI << ss.str();
            params_list.push_back(codec_params);
        }
    }
    return params_list;
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