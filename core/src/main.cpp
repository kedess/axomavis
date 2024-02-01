#include <iostream>

#include <plog/Log.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

extern "C"{
    #include <libavdevice/avdevice.h>
}

#include "version.h"

int main() {
    plog::init(plog::debug);
    plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::get()->addAppender(&consoleAppender);

    LOGI << "Started app Axomavis version (" << VERSION_APP << ")";

    LOGI << "FFMPEG version (" << av_version_info() << ")";
    avdevice_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_WARNING);

    LOGI << "Finished app Axomavis";
    return 0;
}