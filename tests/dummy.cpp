#include <gtest/gtest.h>

class DummyTest : public ::testing::Test {

public:

    DummyTest() {
    }

    ~DummyTest() {
    }

    void SetUp() {

    }

    void TearDown() {
    }
};

TEST_F(DummyTest, Dummy) {
    ASSERT_EQ(true, true);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}