#pragma once

#include <stdexcept>
extern "C"{
    #include <libavformat/avformat.h>
}
#include <plog/Log.h>
namespace axomavis {
    class AVFormatInput{
        public:
            AVFormatInput(){
                ctx = avformat_alloc_context();
                if (ctx == nullptr) {
                    throw std::runtime_error("Failed to create AVFormatContext");
                }
            }
            ~AVFormatInput() {
                if(ctx) {
                    avformat_close_input(&ctx);
                    avformat_free_context(ctx);
                }
            }
            AVFormatInput(const AVFormatInput &p) = delete;
            AVFormatInput& operator=(const AVFormatInput&) = delete;
            AVFormatInput(AVFormatInput &&p) = delete;
            AVFormatInput& operator=(AVFormatInput&&) = delete;
            AVFormatContext * getContext() {
                return ctx;
            }
            void resetContext() {
                ctx = nullptr;
            }
        private:
            AVFormatContext * ctx = nullptr;
    };
    class AVFormatOutput{
        public:
            AVFormatOutput(const char * filename, std::vector<AVCodecParameters*> & codec_params_list){
                if (avformat_alloc_output_context2(&ctx, nullptr, nullptr, filename) < 0){
                    throw std::runtime_error("Failed to create AVFormatContext");
                }
                for (auto codec_params : codec_params_list) {
                    auto out_stream = avformat_new_stream(ctx, nullptr);
                    if(!out_stream) {
                        throw std::runtime_error("Failed allocating output stream");
                    }
                    if (avcodec_parameters_copy((*out_stream).codecpar, codec_params) < 0) {
                        throw std::runtime_error("Failed to copy codec parameters");
                    }
                }
                if (avio_open(&ctx->pb, filename, AVIO_FLAG_WRITE) < 0) {
                    std::stringstream ss;
                    ss << "Could not open output file [" << filename << "]";
                    throw std::runtime_error(ss.str());
                }
                if (avformat_write_header(ctx, nullptr) < 0) {
                    std::stringstream ss;
                    ss << "Error occurred when opening output file [" << filename << "]";
                    throw std::runtime_error(ss.str());
                }
                is_header_wrote = true;
            }
            ~AVFormatOutput() {
                if(ctx) {
                    if (is_header_wrote) {
                        av_write_trailer(ctx);
                    }
                    if (ctx->pb) {
                        avio_close(ctx->pb);
                    }
                    avformat_free_context(ctx);
                }
            }
            AVFormatOutput(const AVFormatOutput &p) = delete;
            AVFormatOutput& operator=(const AVFormatOutput&) = delete;
            AVFormatOutput(AVFormatOutput &&p) = delete;
            AVFormatOutput& operator=(AVFormatOutput&&) = delete;
            AVFormatContext * getContext() {
                return ctx;
            }
        private:
            AVFormatContext * ctx = nullptr;
            bool is_header_wrote = false;
    };
}
