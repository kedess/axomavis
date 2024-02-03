#include <iostream>
#include <thread>
#include <csignal>
#include <plog/Log.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

extern "C"{
    #include <libavdevice/avdevice.h>
}

#include "version.h"
#include "capture/capture.h"

volatile std::sig_atomic_t signal_num = -1;
void siginthandler(int param) {
    signal_num = param;
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

    {
        axomavis::Capture capture("camera-1", "rtsp://admin:admin@192.168.0.1:554/stream");
        std::thread th(&axomavis::Capture::run, &capture);
        th.join();
    }

    LOGI << "Finished app Axomavis";
    return 0;
}