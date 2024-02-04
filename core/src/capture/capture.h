#pragma once

#include "../utils/avformat.h"
#include <vector>
#include <chrono>

namespace axomavis {
    class Capture {
        public:
            Capture(const char * id, const char * url);
            void run() noexcept;
            std::chrono::steady_clock::time_point getTimePoint() const;
            void updateTimePoint();
        private:
            std::vector<AVCodecParameters*> fetch_params(axomavis::AVFormatInput & fmt);
        private:
            const char * id;
            const char * url;
            int video_stream_index = -1;
            int audio_stream_index = -1;
            std::chrono::steady_clock::time_point time_point = std::chrono::steady_clock::now();
    };
}