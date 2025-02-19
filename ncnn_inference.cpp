#include <iostream>
#include <vector>
#include <string>
#include "net.h" // NCNN
#include "ncnn_inference.h"

// Simple image loading (replace with OpenCV or similar if available)
// This is a placeholder, and might not work directly.
//  It's better to use a library like OpenCV.

// Placeholder image loading function (replace with a proper implementation)
//Image loadImage(const std::string& path) {
//    Image img;
    // Replace this with actual image loading using a library like OpenCV
//    std::cerr << "Image loading not implemented. Please use OpenCV or another image library." << std::endl;
//    exit(1);
//    return img;
//}
#define MAX_STRIDE 32

static float softmax(
	const float* src,
	float* dst,
	int length
)
{
	float alpha = -FLT_MAX;
	for (int c = 0; c < length; c++)
	{
		float score = src[c];
		if (score > alpha)
		{
			alpha = score;
		}
	}

	float denominator = 0;
	float dis_sum = 0;
	for (int i = 0; i < length; ++i)
	{
		dst[i] = expf(src[i] - alpha);
		denominator += dst[i];
	}
	for (int i = 0; i < length; ++i)
	{
		dst[i] /= denominator;
		dis_sum += i * dst[i];
	}
	return dis_sum;
}

static void generate_proposals(
	const ncnn::Mat& feat_blob,
	const float prob_threshold,
	std::vector<Object>& objects
)
{
    const int stride = 32; //8, 16 or 32
	const int reg_max = 16;
	float dst[16];
	const int num_w = feat_blob.w;
	const int num_grid_y = feat_blob.c;
	const int num_grid_x = feat_blob.h;

	const int num_class = num_w - 4 * reg_max;

	for (int i = 0; i < num_grid_y; i++)
	{
		for (int j = 0; j < num_grid_x; j++)
		{

			const float* matat = feat_blob.channel(i).row(j);

			int class_index = 0;
			float class_score = -FLT_MAX;
			for (int c = 0; c < num_class; c++)
			{
				float score = matat[4 * reg_max + c];
				if (score > class_score)
				{
					class_index = c;
					class_score = score;
				}
			}

			if (class_score >= prob_threshold)
			{

				float x0 = j + 0.5f - softmax(matat, dst, 16);
				float y0 = i + 0.5f - softmax(matat + 16, dst, 16);
				float x1 = j + 0.5f + softmax(matat + 2 * 16, dst, 16);
				float y1 = i + 0.5f + softmax(matat + 3 * 16, dst, 16);

				x0 *= stride;
				y0 *= stride;
				x1 *= stride;
				y1 *= stride;

				Object obj;
				obj.rect.x = x0;
				obj.rect.y = y0;
				obj.rect.width = x1 - x0;
				obj.rect.height = y1 - y0;
				obj.label = class_index;
				obj.prob = class_score;
				objects.push_back(obj);

			}
		}
	}
}

void perform_inference(const cv::Mat& bgr) {
    // Load NCNN model
    ncnn::Net net;
    net.opt.use_vulkan_compute = true;

    if (net.load_param("code/yolo11n_ncnn_model/model.ncnn.param") != 0) {
        std::cerr << "Failed to load param" << std::endl;
        return;
    }
    if (net.load_model("code/yolo11n_ncnn_model/model.ncnn.bin") != 0) {
        std::cerr << "Failed to load model" << std::endl;
        return;
    }

    const int target_size = 640;
	const float prob_threshold = 0.25f;
	const float nms_threshold = 0.45f;

    int img_w = bgr.cols;
	int img_h = bgr.rows;

	// letterbox pad to multiple of MAX_STRIDE
	int w = img_w;
	int h = img_h;
	float scale = 1.f;
	if (w > h)
	{
		scale = (float)target_size / w;
		w = target_size;
		h = h * scale;
	}
	else
	{
		scale = (float)target_size / h;
		h = target_size;
		w = w * scale;
	}

	ncnn::Mat in = ncnn::Mat::from_pixels_resize(bgr.data, ncnn::Mat::PIXEL_BGR2RGB, img_w, img_h, w, h);

    // pad to target_size rectangle
	// ultralytics/yolo/data/dataloaders/v5augmentations.py letterbox
	int wpad = (w + MAX_STRIDE - 1) / MAX_STRIDE * MAX_STRIDE - w;
	int hpad = (h + MAX_STRIDE - 1) / MAX_STRIDE * MAX_STRIDE - h;

	int top = hpad / 2;
	int bottom = hpad - hpad / 2;
	int left = wpad / 2;
	int right = wpad - wpad / 2;

	ncnn::Mat in_pad;
	ncnn::copy_make_border(in,
		in_pad,
		top,
		bottom,
		left,
		right,
		ncnn::BORDER_CONSTANT,
		114.f);

    // Normalization values
    const float norm_vals[3] = { 1 / 255.f, 1 / 255.f, 1 / 255.f };
	in_pad.substract_mean_normalize(0, norm_vals);

    ncnn::Extractor ex = net.create_extractor();
    
    ex.input("in0", in_pad);

    ncnn::Mat out;

    ex.extract("out0", out);
    
    std::vector<Object> objects;
    
    generate_proposals(out, prob_threshold, objects);

    static const char* class_names[] = {
		"person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
		"fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
		"elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
		"skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
		"tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
		"sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
		"potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
		"microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
		"hair drier", "toothbrush"
	};
    for (size_t i = 0; i < objects.size(); i++)
	{
		const Object& obj = objects[i];
        //TODO: avoid accessing class_names[obj.label] without checking size to avoid out of bound errors
        char text[256];
		sprintf(text, "%s %.1f%%", class_names[obj.label], obj.prob * 100);
		fprintf(stderr, "%s = %.5f at %.2f %.2f %.2f x %.2f\n", class_names[obj.label], obj.prob,
			obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height);
    }
}
