#pragma once

#pragma once

extern "C"{
    #include <libswscale/swscale.h>
}
#include "../utils/avframe.h"

namespace axomavis {
    class VisualizerInferences final {
        public:
            VisualizerInferences() = default;
            ~VisualizerInferences() = default;
            VisualizerInferences(const VisualizerInferences &p) = delete;
            VisualizerInferences& operator=(const VisualizerInferences&) = delete;
            VisualizerInferences(VisualizerInferences &&p) = delete;
            VisualizerInferences& operator=(VisualizerInferences&&) = delete;
            AVFrameWrapper render_nv12(AVFrameWrapper & frame);
            AVFrameWrapper render_yuv(AVFrameWrapper & frame);
    };
}