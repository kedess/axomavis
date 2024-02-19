#include "capture.h"
#include <plog/Log.h>
#include <csignal>
#include <cstring>
#include <sstream>
#include <thread>
#include "../utils/avformat.h"
#include "../utils/avpacket.h"
#include "../archive/archive.h"
#include "../decoder/decoder.h"

extern "C"{
    #include <libavformat/avformat.h>
}

struct CodecParams {
    int index;
    AVCodecID id;
    const AVCodec * codec;
    const AVCodecParameters * codec_params_list;
    bool is_video_stream;
};

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
            auto fmt_in_ctx = fmt_in.getContext();
            /*
            * Функция следит за временем задержки получения пакетов из потока,
            * используя переменную time_point
            */
            AVIOInterruptCB icb= {check_latency_read_packet,reinterpret_cast<void*>(this)};
            fmt_in_ctx->interrupt_callback = icb;
            fmt_in_ctx->interrupt_callback.opaque = this;
            if (avformat_open_input(&fmt_in_ctx, source.getUrl().c_str(), nullptr, nullptr) < 0) {
                /*
                * Обнуляем обязательно указатель, так как в случаи сбоя avformat_open_input,
                * функция очищает данные указателя fmt_in_ctx
                */
                fmt_in.resetContext();
                std::stringstream ss;
                ss << "Unable to connect to the video stream [" << source.getId() << "]";
                throw std::runtime_error(ss.str());
            }
            if (avformat_find_stream_info(fmt_in_ctx, nullptr) < 0) {
                std::stringstream ss;
                ss << "Not found info for stream [" << source.getId() << "]";
                throw std::runtime_error(ss.str());
            }
            auto params_list = fetch_params(fmt_in);
            LOGI << "The stream has been successfully launched [" << source.getId() << "]";

            /*
            * Параметры для инициализация декодера
            */
            const AVCodec * video_codec = nullptr;
            int video_stream_index = -1;
            const AVCodecParameters * video_codec_params = nullptr;

            std::vector<const AVCodecParameters *> codec_params_list;
            for(auto & codec_params: params_list) {
                if (codec_params.is_video_stream) {
                    video_codec = codec_params.codec;
                    video_stream_index = codec_params.index;
                    video_codec_params = codec_params.codec_params_list;
                }
                codec_params_list.push_back(codec_params.codec_params_list);
            }
            axomavis::Archive archive(codec_params_list, source.getId().c_str());
            axomavis::AVPacketWrapper packet;
            
            /*
            * Соединение прошло успешно, потоки определены выставляем состояние RUNNING
            */
            set_running_state();
            auto av_packet = packet.getAVPacket();
            if (!video_codec || !video_codec_params) {
                throw std::runtime_error("No initializers found for the video codec");
            }
            Decoder decoder(video_codec, video_codec_params);
            while (signal_num == -1 && av_read_frame(fmt_in_ctx, av_packet) == 0) {
                updateTimePoint();
                if (av_packet->stream_index == video_stream_index) {
                    switch (*(video_codec->pix_fmts)) {
                        case AVPixelFormat::AV_PIX_FMT_CUDA: {
                            auto frame_decoded = decoder.decode_gpu(packet);
                            if (frame_decoded.has_value()) {
                                LOGD << "Decoded frame gpu";
                            }
                            break;
                        }
                        case AVPixelFormat::AV_PIX_FMT_YUV420P: {
                            auto frame_decoded = decoder.decode_cpu(packet);
                            if (frame_decoded.has_value()) {
                                LOGD << "Decoded frame cpu";
                            }
                            break;
                        }
                        default:{
                            throw std::runtime_error("Not supported Pixel Format");
                            break;
                        }
                    }
                }
                archive.recv_pkt(packet, fmt_in);
                av_packet_unref(av_packet);
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

std::vector<CodecParams> axomavis::Capture::fetch_params(axomavis::AVFormatInput & fmt_in) {
    std::vector<CodecParams> codec_params_list;
    auto fmt_in_ctx = fmt_in.getContext();
    for (size_t idx = 0; idx < fmt_in_ctx->nb_streams; idx++) {
        auto codec_params = fmt_in_ctx->streams[idx]->codecpar;
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
                            codec_params_list.push_back(
                                CodecParams{static_cast<int>(idx), AVCodecID::AV_CODEC_ID_H264, codec, codec_params, true}
                            );
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
                            codec_params_list.push_back(
                                CodecParams{static_cast<int>(idx), AVCodecID::AV_CODEC_ID_HEVC, codec, codec_params, true}
                            );
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
            codec_params_list.push_back(
                CodecParams{static_cast<int>(idx), codec_params->codec_id, nullptr, codec_params, false}
            );
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