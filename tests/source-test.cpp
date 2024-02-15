#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "../core/src/source/source.h"
using json = nlohmann::json;

class SourceTest : public ::testing::Test {

public:

    SourceTest() {
    }

    ~SourceTest() {
    }

    void SetUp() {

    }

    void TearDown() {
    }
};

TEST_F(SourceTest, ParseJsonFromString) {
    auto text = R"(
        {
            "id":"camera-1",
            "url":"rtsp://admin:admin@192.168.0.1:554/stream"
        }
    )";
    json j_complete = json::parse(text);
    ASSERT_EQ(j_complete["id"].get<std::string>(), "camera-1");
    ASSERT_EQ(j_complete["url"].get<std::string>(), "rtsp://admin:admin@192.168.0.1:554/stream");
}
TEST_F(SourceTest, SourceParseFromString) {
    auto text = R"(
        [
            {
                "id":"camera-1",
                "url":"rtsp://admin:admin@192.168.0.1:554/stream"
            },
            {
                "id":"camera-2",
                "url":"rtsp://admin:admin@192.168.0.2:554/stream"
            }
        ]
    )";
    std::vector<const char *> ids {
        "camera-1",
        "camera-2"
    };
    std::vector<const char *> urls {
        "rtsp://admin:admin@192.168.0.1:554/stream",
        "rtsp://admin:admin@192.168.0.2:554/stream"
    };
    json j_complete = json::parse(text);
    size_t idx = 0;
    for(auto value : j_complete) {
        axomavis::Source source = axomavis::Source::from_json(value);
        ASSERT_EQ(source.getId(), ids[idx]);
        ASSERT_EQ(source.getUrl(), urls[idx]);
        idx++;
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}