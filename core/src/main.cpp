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
#include <deque>

using namespace std::chrono_literals;

extern "C"{
    #include <libavdevice/avdevice.h>
}

#include "version.h"
#include "capture/capture.h"
#include "processing/processing.h"
#include <nlohmann/json.hpp>

#include "source/source.h"
using json = nlohmann::json;

std::condition_variable is_stop_app_cond;
std::mutex is_stop_app_mutex;

/*
* Очередь сохраненных видео файлов, которые пойдут на видео аналитику
*/
std::mutex queue_video_files_mutex;
std::condition_variable queue_video_files_cond;
std::deque<std::string> queue_video_files;

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

    std::thread processing_thread([](){
        while (signal_num == -1) {
            auto now = std::chrono::steady_clock::now();
            std::unique_lock<std::mutex> lock(queue_video_files_mutex);
            queue_video_files_cond.wait_until(lock, now + 5s);
            if (!queue_video_files.empty()) {
                auto filename = queue_video_files.front();
                queue_video_files.pop_front();
                lock.unlock();

                LOGD << "Started processing file [" << filename << "]";
                /*
                * Обработка видео файла
                */
                try {
                    axomavis::Processing processing(filename.c_str());
                } catch (const std::exception & ex) {
                    LOGE << ex.what();
                }
            }
        }
    });

    std::vector<std::thread> threads;
    std::vector<axomavis::Source> sources;
    try {
        sources = std::move(axomavis::Source::from_file("/home/amazing-hash/sources.json"));
    } catch(const std::exception & ex) {
        LOGE << "Error parsing sources json file";
        signal_num = SIGQUIT;
    }

    size_t nstreams_success = 0;
    for (auto source : sources) {
        axomavis::Capture capture (source);
        if (capture.getStateType() != axomavis::Error) {
            nstreams_success++;
            threads.emplace_back(&axomavis::Capture::run, std::move(capture)); 
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
        signal_num = SIGQUIT;
    }
    if (processing_thread.joinable()){
        processing_thread.join();
    }

    LOGI << "Finished app Axomavis";
    return 0;
}