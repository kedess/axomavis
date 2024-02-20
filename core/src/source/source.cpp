#include "source.h"
#include <iostream>
#include <fstream>

namespace axomavis {
    const std::string & Source::getId() const & {
        return id;
    }
    const std::string & Source::getUrl() const & {
        return url;
    }
    Source::Source(std::string id, std::string url) : id(id), url(url){

    }
    Source Source::from_json(const json & value) {
        Source source(value["id"].get<std::string>(), value["url"].get<std::string>());
        return source;
    }
    std::vector<Source> Source::from_file(const char * filename) {
        std::ifstream file(filename);
        std::ostringstream sstr;
        sstr << file.rdbuf();
        json j_complete = json::parse(sstr.str());
        std::vector<Source> sources;
        for (auto value : j_complete) {
            sources.push_back(Source::from_json(value));
        }
        return sources;
    }
}