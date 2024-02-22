#include "visualizer.h"

#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdexcept>

namespace axomavis {
    AVFrameWrapper VisualizerInferences::render_yuv(AVFrameWrapper & frame) {
        auto width = frame->width;
        auto height = frame->height;
        cv::Mat img_yuv420 = cv::Mat::zeros( (height * 3) >> 1, width, CV_8UC1 );    
        memcpy(img_yuv420.data, frame->data[0], static_cast<size_t>(width * height));
        memcpy(img_yuv420.data + width * height, frame->data[1], 
                static_cast<size_t>((width * height)) >> 2);
        memcpy(img_yuv420.data + ((width * height * 5) >> 2), frame->data[2],
                static_cast<size_t>((width * height)) >> 2);

        cv::Mat img;
        cv::cvtColor(img_yuv420, img, cv::COLOR_YUV2BGR_I420);

        /*
        * Test rectangle render
        */
        cv::Rect rect(100, 100, 300, 300);
        cv::rectangle(img, rect, cv::Scalar(0, 128, 0), 2);
        // cv::imwrite("test.jpg", img);
        
        AVFrameWrapper new_frame;
        new_frame -> width = width;
        new_frame -> height = height;
        new_frame -> format = AV_PIX_FMT_YUV420P;

        if (av_frame_get_buffer(new_frame.get(), 0) < 0) {
            throw std::runtime_error("Not allocated buffer av_frame");
        }
        if (av_frame_make_writable(new_frame.get()) < 0){
            throw std::runtime_error("Not make av_frame writable");
        }

        cv::cvtColor(img, img, cv::COLOR_BGR2YUV_I420);

        memcpy(new_frame->data[0], img.data, static_cast<size_t>(width * height));
        memcpy(new_frame->data[1], img.data + width * height, static_cast<size_t>((width * height) >> 2));
        memcpy(new_frame->data[2], img.data + static_cast<size_t>((5 * width * height) >> 2), static_cast<size_t>((width * height) >> 2));

        return new_frame;
    }
    AVFrameWrapper VisualizerInferences::render_nv12(AVFrameWrapper & frame) {
        auto width = frame->width;
        auto height = frame->height;
        
        cv::Mat img_nv12 = cv::Mat::zeros((height * 3) >> 1, width, CV_8UC1);   
        memcpy(img_nv12.data, frame->data[0], static_cast<size_t>(width * height));
        memcpy(img_nv12.data + width * height, frame->data[1], static_cast<size_t>((width * height) >> 1));

        cv::Mat img;
        cv::cvtColor(img_nv12, img, cv::COLOR_YUV2BGR_NV12);

        /*
        * Test rectangle render
        */
        cv::Rect rect(100, 100, 300, 300);
        cv::rectangle(img, rect, cv::Scalar(0, 128, 0), 2);
        // cv::imwrite("test.jpg", img);

        AVFrameWrapper new_frame;
        new_frame -> width = width;
        new_frame -> height = height;
        new_frame -> format = AV_PIX_FMT_YUV420P;
        if (av_frame_get_buffer(new_frame.get(), 0) < 0) {
            throw std::runtime_error("Not allocated buffer av_frame");
        }
        if (av_frame_make_writable(new_frame.get()) < 0){
            throw std::runtime_error("Not make av_frame writable");
        }
        
        cv::cvtColor(img, img, cv::COLOR_BGR2YUV_I420);
        memcpy(new_frame->data[0], img.data, static_cast<size_t>(width * height));
        memcpy(new_frame->data[1], img.data + width * height, static_cast<size_t>((width * height) >> 2));
        memcpy(new_frame->data[2], img.data + static_cast<size_t>((5 * width * height) >> 2), static_cast<size_t>((width * height) >> 2));
        return new_frame;
    }
}