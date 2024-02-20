#pragma once

#include <chrono>
#include <vector>
#include <string>

namespace axomavis {
    class Processing {
        public:
            Processing(const char * input_filename);
            ~Processing() = default;
            Processing(const Processing &p) = delete;
            Processing& operator=(const Processing&) = delete;
            Processing(Processing &&p) = delete;
            Processing& operator=(Processing&&) = delete;
        private:
            std::string output_filename;
    };
}