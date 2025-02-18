#include <iostream>
#include <vector>
#include <string>
#include "net.h" // NCNN

// Simple image loading (replace with OpenCV or similar if available)
// This is a placeholder, and might not work directly.
//  It's better to use a library like OpenCV.
struct Pixel {
    unsigned char r, g, b;
};

struct Image {
    int width, height;
    std::vector<Pixel> pixels;
};

// Placeholder image loading function (replace with a proper implementation)
Image loadImage(const std::string& path) {
    Image img;
    // Replace this with actual image loading using a library like OpenCV
    std::cerr << "Image loading not implemented. Please use OpenCV or another image library." << std::endl;
    exit(1);
    return img;
}

// Hardcoded COCO labels (replace with loading from a file if available)
const std::vector<std::string> coco_labels = {
    "person", "bicycle", "car", "motorcycle", "airplane",
    "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench",
    "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack",
    "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat",
    "baseball glove", "skateboard", "surfboard", "tennis racket",
    "bottle", "wine glass", "cup", "fork", "knife", "spoon",
    "bowl", "banana", "apple", "sandwich", "orange",
    "broccoli", "carrot", "hot dog", "pizza", "donut",
    "cake", "chair", "couch", "potted plant", "bed",
    "dining table", "toilet", "tv", "laptop", "mouse",
    "remote", "keyboard", "cell phone", "microwave", "oven",
    "toaster", "sink", "refrigerator", "book", "clock",
    "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
};

int main() {
    // Load NCNN model
    ncnn::Net net;
    if (net.load_param("code/yolo11n_ncnn_model/model.ncnn.param") != 0) {
        std::cerr << "Failed to load param" << std::endl;
        return -1;
    }
    if (net.load_model("code/yolo11n_ncnn_model/model.ncnn.bin") != 0) {
        std::cerr << "Failed to load model" << std::endl;
        return -1;
    }

    // Load image
    Image image = loadImage("code/bus.jpg");

    // Preprocess image (assuming model expects 640x640 input)
    int target_size = 640;
    //  (In a real implementation, you'd resize and normalize here)

    // Create extractor
    ncnn::Extractor ex = net.create_extractor();

    // Set input (replace with actual preprocessed image data)
    ncnn::Mat in = ncnn::Mat::from_pixels_resize(
        (const unsigned char*)image.pixels.data(),
        ncnn::Mat::PIXEL_RGB, // Assuming RGB format
        image.width,
        image.height,
        target_size,
        target_size
    );

  // Normalization values from the Pytorch model
    const float mean_vals[3] = {0.f, 0.f, 0.f};
    const float norm_vals[3] = {1/255.f, 1/255.f, 1/255.f};
    in.substract_mean_normalize(mean_vals, norm_vals);

    ex.input("in0", in); // Assuming "in0" is the input layer name

    // Perform inference
    ncnn::Mat out;
    ex.extract("out0", out); // Assuming "out0" is the output layer name

    // Process output (This depends on the model's output format)
    // Assuming output is a 1xN matrix where N is the number of detections
    // and each detection is [x, y, w, h, confidence, class1, class2, ...]

    for (int i = 0; i < out.w; i++) {
        const float* values = out.row(i);

        float x = values[0];
        float y = values[1];
        float w = values[2];
        float h = values[3];
        float confidence = values[4];
        int class_id = 0;
        float max_prob = 0.0f;

        // Find the class with the highest probability
        for (int j = 5; j < out.c; j++) {
          if (values[j] > max_prob)
          {
            max_prob = values[j];
            class_id = j - 5;
          }
        }

        if (confidence > 0.5) { // Threshold
            std::cout << "Detected: " << coco_labels[class_id]
                      << " with confidence " << confidence
                      << " at (" << x << ", " << y << ", " << w << ", " << h << ")"
                      << std::endl;
        }
    }

    return 0;
}
