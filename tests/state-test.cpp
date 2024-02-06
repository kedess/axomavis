#include <gtest/gtest.h>
#include <memory.h>
#include "../core/src/state/state.h"

class StateTest : public ::testing::Test {

public:

    StateTest() {
    }

    ~StateTest() {
    }

    void SetUp() {

    }

    void TearDown() {
    }
};

TEST_F(StateTest, StatePending1) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamPendingState);
    state->set_pending_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Pending);
}
TEST_F(StateTest, StatePending2) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamPendingState);
    state->set_running_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Running);
}
TEST_F(StateTest, StatePending3) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamPendingState);
    state->set_error_state(state, "description");
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Error);
}

TEST_F(StateTest, StateRunning1) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamRunningState);
    state->set_pending_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Running);
}
TEST_F(StateTest, StateRunning2) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamRunningState);
    state->set_running_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Running);
}
TEST_F(StateTest, StateRunning3) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamRunningState);
    state->set_error_state(state, "description");
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Running);
}

TEST_F(StateTest, StateError1) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamErrorState("description"));
    state->set_pending_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Error);
}
TEST_F(StateTest, StateError2) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamErrorState("description"));
    state->set_running_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Error);
}
TEST_F(StateTest, StateError3) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamErrorState("description"));
    state->set_error_state(state, "description continue");
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Error);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}