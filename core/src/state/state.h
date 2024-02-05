#pragma once
#include <string>
#include <memory>

namespace axomavis {
    enum StreamStateEnum {
        Pending,
        Running, Error
    };
    class StreamState {
        public:
            virtual void set_pending_state(std::unique_ptr<StreamState>& state) = 0;
            virtual void set_running_state(std::unique_ptr<StreamState>& state) = 0;
            virtual void set_error_state(std::unique_ptr<StreamState>& state, const char * description) = 0;
            virtual StreamStateEnum get_type() const = 0;
            virtual ~StreamState() = default;
    };
    class StreamPendingState: public StreamState {
        public:
            virtual void set_pending_state(std::unique_ptr<StreamState>& state) override;
            virtual void set_running_state(std::unique_ptr<StreamState>& state) override;
            virtual void set_error_state(std::unique_ptr<StreamState>& state, const char * description) override;
            virtual StreamStateEnum get_type() const override;
        private:
    };
    class StreamRunningState: public StreamState {
        public:
            virtual void set_pending_state(std::unique_ptr<StreamState>& state) override;
            virtual void set_running_state(std::unique_ptr<StreamState>& state) override;
            virtual void set_error_state(std::unique_ptr<StreamState>& state,const char * description) override;
            virtual StreamStateEnum get_type() const override;
        private:
    };
    class StreamErrorState: public StreamState {
        public:
            StreamErrorState(const char * description);
            virtual void set_pending_state(std::unique_ptr<StreamState>& state) override;
            virtual void set_running_state(std::unique_ptr<StreamState>& state) override;
            virtual void set_error_state(std::unique_ptr<StreamState>& state, const char * description) override;
            virtual StreamStateEnum get_type() const override;
            const char * getDescription() const;
        private:
            std::string description;
    };
}