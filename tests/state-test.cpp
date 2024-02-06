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

TEST_F(StateTest, TransitionPendingToPending) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamPendingState);
    state->set_pending_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Pending);
}
TEST_F(StateTest, TransitionPendingToRunning) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamPendingState);
    state->set_running_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Running);
}
TEST_F(StateTest, TransitionPendingToError) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamPendingState);
    state->set_error_state(state, "description");
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Error);
}

TEST_F(StateTest, TransitionRunningToPending) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamRunningState);
    state->set_pending_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Pending);
}
TEST_F(StateTest, TransitionRunningToRunning) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamRunningState);
    state->set_running_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Running);
}
TEST_F(StateTest, TransitionRunningToError) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamRunningState);
    state->set_error_state(state, "description");
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Running);
}

TEST_F(StateTest, TransitionErrorToPending) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamErrorState("description"));
    state->set_pending_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Error);
}
TEST_F(StateTest, TransitionErrorToRunning) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamErrorState("description"));
    state->set_running_state(state);
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Error);
}
TEST_F(StateTest, TransitionErrorToError) {
    std::unique_ptr<axomavis::StreamState> state(new axomavis::StreamErrorState("description"));
    state->set_error_state(state, "description continue");
    ASSERT_EQ(state->get_type(), axomavis::StreamStateEnum::Error);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}