#include <iostream>
#include <fstream>
#include <chrono>
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/opencv.hpp>

void save_jpeg(cv::Mat save_img) {
    // Compress the image using libjpeg-turbo.
        // Generate filename based on current timestamp.
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        char filename[64];
        std::strftime(filename, sizeof(filename), "%Y%m%d_%H%M%S.jpg", std::localtime(&now_time));

       cv::imwrite(filename, save_img);
}