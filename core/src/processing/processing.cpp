#include "processing.h"
#include "../utils/avformat.h"
#include "../decoder/decoder.h"
#include "../encoder/encoder.h"
#include "../visualizer/visualizer.h"
#include "../writer/writer.h"

struct VideoCodecParams {
    int index;
    AVCodecID id;
    const AVCodec * codec;
    const AVCodecParameters *codec_params;
};

static const VideoCodecParams fetch_video_codec_params(axomavis::AVFormatInput & fmt_in, const char * input_filename) {
    for (int idx = 0; idx < static_cast<int>(fmt_in->nb_streams); idx++) {
        auto codec_params = fmt_in->streams[idx]->codecpar;
        auto media_type = codec_params->codec_type;
        if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
            switch (codec_params->codec_id) {
                case AVCodecID::AV_CODEC_ID_H264: {
                    const char * names[] = {"h264_cuvid", "h264"};
                    for(auto name : names) {
                        auto codec = avcodec_find_decoder_by_name(name);
                        if (codec) {
                            return {idx, AVCodecID::AV_CODEC_ID_H264, codec, codec_params};
                        }
                    }
                    std::stringstream ss;
                    ss << "Not found video decoder [" << avcodec_get_name(codec_params->codec_id)
                       << "] for video file [" << input_filename << "]";
                    throw std::runtime_error(ss.str());
                }
                case AVCodecID::AV_CODEC_ID_HEVC: {
                    const char * names[] = {"hevc_cuvid", "h265", "hevc"};
                    for(auto name : names) {
                        auto codec = avcodec_find_decoder_by_name(name);
                        if (codec) {
                            return {idx, AVCodecID::AV_CODEC_ID_H264, codec, codec_params};
                        }
                    }
                    std::stringstream ss;
                    ss << "Not found video decoder [" << avcodec_get_name(codec_params->codec_id)
                       << "] for video file [" << input_filename << "]";
                    throw std::runtime_error(ss.str());
                }
                default: {
                    std::stringstream ss;
                    ss << "Unsupported video decoder [" << avcodec_get_name(codec_params->codec_id)
                       << "] for video file [" << input_filename << "]";
                    throw std::runtime_error(ss.str());
                }
            }
        }
    }
    return {-1, AVCodecID::AV_CODEC_ID_NONE, nullptr, nullptr};
}

namespace axomavis {
    Processing::Processing(const char * input_filename){
        AVFormatInput fmt_in;
        auto ptr = fmt_in.get();
        if (avformat_open_input(&ptr, input_filename, nullptr, nullptr) < 0) {
            /*
            * Обнуляем обязательно указатель, так как в случаи сбоя avformat_open_input,
            * функция очищает данные указателя fmt_in_ctx
            */
            fmt_in.resetContext();
            std::stringstream ss;
            ss << "Unable to read video file [" << input_filename << "]";
            throw std::runtime_error(ss.str());
        }
        if (avformat_find_stream_info(fmt_in.get(), nullptr) < 0) {
            std::stringstream ss;
            ss << "Not found info for stream file [" << input_filename << "]";
            throw std::runtime_error(ss.str());
        }
        auto[video_index, video_codec_id, video_codec, codec_params] = fetch_video_codec_params(fmt_in, input_filename);

        if (video_index == -1) {
            std::stringstream ss;
            ss << "Not found video stream into file [" << input_filename << "]";
            throw std::runtime_error(ss.str());
        }
        AVPacketWrapper packet;
        Decoder decoder(video_codec, codec_params);
        Encoder encoder(codec_params, *video_codec->pix_fmts);
        VisualizerInferences visualizer;
        /*
        * Имя файла для примера
        */
        Writer writer(video_codec, "/tmp/axomavis/inference.ts");
        while (av_read_frame(fmt_in.get(), packet.get()) == 0) {
            if (packet->stream_index == video_index) {
                switch (*(video_codec->pix_fmts)) {
                    case AVPixelFormat::AV_PIX_FMT_CUDA: {
                        auto frame_decoded = decoder.decode_gpu(packet);
                        if (frame_decoded.has_value()) {
                            auto frame_inference = visualizer.render_nv12(frame_decoded.value());
                            AVFrameWrapper hw_frame;
                            av_hwframe_get_buffer(encoder.get_hw_frames_ref(), hw_frame.get(), 0);
                                if (av_hwframe_transfer_data(hw_frame.get(), frame_inference.get(), 0) == 0) {
                                    auto packets = encoder.encode_gpu(hw_frame.get());
                                    for(auto & packet : packets) {
                                        packet->pts = frame_decoded.value()->pts;
                                        writer.recv_pkt(packet);
                                    }
                                }
                        }
                        break;
                    }
                    case AVPixelFormat::AV_PIX_FMT_YUV420P: {
                        auto frame_decoded = decoder.decode_cpu(packet);
                        if (frame_decoded.has_value()) {
                            auto frame_inference = visualizer.render_yuv(frame_decoded.value());
                            auto packets = encoder.encode_cpu(frame_inference.get());
                            for(auto & packet : packets) {
                                packet->pts = frame_decoded.value()->pts;
                                writer.recv_pkt(packet);
                            }
                        }
                        break;
                    }
                    default:{
                        throw std::runtime_error("Not supported Pixel Format");
                        break;
                    }
                }
            }
            av_packet_unref(packet.get());
        }
    }
}