#include "state.h"
#include <assert.h>

void axomavis::StreamPendingState::set_pending_state([[maybe_unused]] std::unique_ptr<StreamState>& state) {
    assert(false);
}
void axomavis::StreamPendingState::set_running_state(std::unique_ptr<StreamState>& state) {
    state.reset(new StreamRunningState);
}
void axomavis::StreamPendingState::set_error_state(std::unique_ptr<StreamState>& state, const char * description) {
    state.reset(new StreamErrorState(description));
}

void axomavis::StreamRunningState::set_pending_state([[maybe_unused]] std::unique_ptr<StreamState>& state) {
    assert(false);
}
void axomavis::StreamRunningState::set_running_state([[maybe_unused]] std::unique_ptr<StreamState>& state) {
    assert(false);
}
void axomavis::StreamRunningState::set_error_state([[maybe_unused]] std::unique_ptr<StreamState>& state,
                                                   [[maybe_unused]] const char * description) {
    assert(false);
}

void axomavis::StreamErrorState::set_pending_state([[maybe_unused]] std::unique_ptr<StreamState>& state) {
    assert(false);
}
void axomavis::StreamErrorState::set_running_state([[maybe_unused]] std::unique_ptr<StreamState>& state) {
    assert(false);
}
void axomavis::StreamErrorState::set_error_state([[maybe_unused]] std::unique_ptr<StreamState>& state,
                                                 [[maybe_unused]] const char * description) {
    assert(false);
}

axomavis::StreamErrorState::StreamErrorState(const char * description) : description(description){

}
const char * axomavis::StreamErrorState::getDescription() const {
    return description.c_str();
}

axomavis::StreamStateEnum axomavis::StreamPendingState::get_type() const {
    return StreamStateEnum::Pending;
}
axomavis::StreamStateEnum axomavis::StreamRunningState::get_type() const {
    return StreamStateEnum::Running;
}
axomavis::StreamStateEnum axomavis::StreamErrorState::get_type() const {
    return StreamStateEnum::Error;
}