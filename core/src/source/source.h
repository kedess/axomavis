#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace axomavis {
    class Source {
        public:
            Source(std::string id, std::string url);
            static std::vector<Source> from_file(const char * filename);
            static Source from_json(const json & value);
            Source(const Source &p) = default;
            Source& operator=(const Source&) = default;
            Source(Source &&p) = default;
            Source& operator=(Source&&) = default;
            const std::string & getId() const &;
            const std::string & getUrl() const &;
        private:
            std::string id;
            std::string url;
    };
}