#ifndef NCNN_INFERENCE_H
#define NCNN_INFERENCE_H

#include <string>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>

// Placeholder image structure (replace with a proper definition if using OpenCV or similar)
//struct Image {
//    std::vector<unsigned char> pixels;
//    int width;
//    int height;
//};
// We don't need the Image struct anymore as we are passing the raw buffer

struct Object
{
	cv::Rect_<float> rect;
	int label;
	float prob;
};

void perform_inference(const cv::Mat& bgr);

#endif
