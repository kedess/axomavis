#pragma once

#include "../utils/avformat.h"
#include "../state/state.h"
#include "../source/source.h"
#include <vector>
#include <chrono>
#include <memory>

namespace axomavis {
    class Capture final {
        public:
            Capture(Source source);
            Capture(const Capture &p) = delete;
            Capture& operator=(const Capture&) = delete;
            Capture(Capture &&p) = default;
            Capture& operator=(Capture&&) = default;
            void run() noexcept;
            std::chrono::steady_clock::time_point getTimePoint() const;
            void updateTimePoint();
            void set_pending_state();
            void set_running_state();
            void set_error_state(const char * description);
            StreamStateEnum getStateType() const;
        private:
            std::vector<const AVCodecParameters *> fetch_input_stream_params(axomavis::AVFormatInput & fmt);
        public:
            static size_t numbers;
        private:
            std::unique_ptr<StreamState> state = nullptr;
            Source source;
            std::chrono::steady_clock::time_point time_point = std::chrono::steady_clock::now();
    };
}