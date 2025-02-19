#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/core.hpp>
#include "ncnn_inference.h"

int main() {
    // Use libcamera with OpenCV's VideoCapture
    cv::VideoCapture cap(cv::CAP_ANY); // CAP_ANY will try to use libcamera automatically

    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera." << std::endl;
        return -1;
    }

    cv::Mat frame;
    cap >> frame; // Capture a frame

    if (frame.empty()) {
        std::cerr << "Error: Captured frame is empty." << std::endl;
        return -1;
    }

    // Perform inference
    perform_inference(frame);

    // Optional: Display the captured frame (for debugging)
    // cv::imshow("Captured Frame", frame);
    // cv::waitKey(0); // Wait for a key press to close the window

    return 0;
}