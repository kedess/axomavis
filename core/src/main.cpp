#include <iostream>
#include <thread>
#include <csignal>
#include <condition_variable>
#include <mutex>
#include <algorithm>
#include <plog/Log.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

extern "C"{
    #include <libavdevice/avdevice.h>
}

#include "version.h"
#include "capture/capture.h"

std::condition_variable is_stop_app_cond;
std::mutex is_stop_app_mutex;

volatile std::sig_atomic_t signal_num = -1;
void siginthandler(int param) {
    signal_num = param;
    std::unique_lock<std::mutex> lock(is_stop_app_mutex);
    is_stop_app_cond.notify_one();
    LOGI << "Received signal SIGINT";
}

int main(/*int argc, char * argv[]*/) {
    plog::init(plog::debug);
    plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::get()->addAppender(&consoleAppender);

    signal(SIGINT, siginthandler);

    LOGI << "Started app Axomavis version (" << VERSION_APP << ")";

    LOGI << "FFMPEG version (" << av_version_info() << ")";
    avdevice_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_QUIET);

    std::vector<std::thread> threads;
    std::vector<axomavis::Capture> captures;

    size_t nstreams_success = 0;
    for (size_t i = 0; i < 1; i++) {
        captures.emplace_back(axomavis::Capture("camera-1", "rtsp://admin:admin@192.168.0.1:554/stream"));
        if (captures.back().getStateType() != axomavis::Error) {
            threads.emplace_back(std::thread(&axomavis::Capture::run, &captures.back())); 
            nstreams_success++;
        }
    }

    if (nstreams_success) {
        LOGI << nstreams_success << "/" << axomavis::Capture::numbers << " cameras will be launched";
        
        std::unique_lock<std::mutex> lock(is_stop_app_mutex);
        is_stop_app_cond.wait(lock, []() {
            return signal_num != -1;
        });

        for (auto & thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    } else {
        LOGI << "There are no cameras to launch";
    }

    LOGI << "Finished app Axomavis";
    return 0;
}