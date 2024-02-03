#pragma once

#include "../utils/avformat.h"
#include <vector>

namespace axomavis {
    class Capture {
        public:
            Capture(const char * id, const char * url);
            void run() noexcept;
        private:
            std::vector<AVCodecParameters*> fetch_params(axomavis::AVFormatInput & fmt);
        private:
            const char * id;
            const char * url;
            int video_stream_index = -1;
            int audio_stream_index = -1;
    };
}