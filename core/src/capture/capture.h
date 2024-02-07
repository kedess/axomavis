#pragma once

#include "../utils/avformat.h"
#include "../state/state.h"
#include <vector>
#include <chrono>
#include <memory>

namespace axomavis {
    class Capture {
        public:
            Capture(const char * id, const char * url);
            void run() noexcept;
            std::chrono::steady_clock::time_point getTimePoint() const;
            void updateTimePoint();
            void set_pending_state();
            void set_running_state();
            void set_error_state(const char * description);
            StreamStateEnum getStateType() const;
        private:
            std::vector<AVCodecParameters*> fetch_params(axomavis::AVFormatInput & fmt);
        public:
            static size_t numbers;
        private:
            std::unique_ptr<StreamState> state = nullptr;
            const char * id;
            const char * url;
            int video_stream_index = -1;
            int audio_stream_index = -1;
            std::chrono::steady_clock::time_point time_point = std::chrono::steady_clock::now();
    };
}