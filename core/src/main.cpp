#include <iostream>
extern "C"{
    #include <libavdevice/avdevice.h>
}


int main() {
    avdevice_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_ERROR);

    std::cout << "Hello Axomavis" << std::endl;
    return 0;
}