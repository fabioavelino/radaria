#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <thread>
#include <cstring>
#include <memory>
#include <signal.h>

// Needed for buffer allocation and mapping.
#include <libcamera/formats.h>
#include <libcamera/camera_manager.h>
#include <libcamera/camera.h>
#include <libcamera/request.h>
#include <libcamera/controls.h>
#include <libcamera/stream.h>
//#include <libcamera/buffer.h>
#include <libcamera/framebuffer_allocator.h>

// libjpeg-turbo header
#include <turbojpeg.h>

using namespace std;
using namespace libcamera;

int main() {
    // Initialize camera manager
    CameraManager manager;
    int ret = manager.start();
    if(ret) {
        cerr << "Failed to start camera manager." << endl;
        return 1;
    }

    if (manager.cameras().empty()) {
        cerr << "No cameras available." << endl;
        manager.stop();
        return 1;
    }

    // Acquire the first available camera
    std::shared_ptr<Camera> camera = manager.cameras()[0];
    if(camera->acquire()) {
        cerr << "Failed to acquire camera." << endl;
        manager.stop();
        return 1;
    }

    // Generate configuration for still capture
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration({ StreamRole::StillCapture });
    if(!config) {
        cerr << "Failed to generate camera configuration." << endl;
        camera->release();
        manager.stop();
        return 1;
    }

    // For simplicity, we choose a single stream and set it for 1920x1080 capture.
    if(!config->empty()) {
        // Choose RGB888 so the buffer can be directly processed via libjpeg-turbo.
        config->at(0).pixelFormat = formats::RGB888;
        config->at(0).size = {1920, 1080};
    } else {
        cerr << "No configuration streams available." << endl;
        camera->release();
        manager.stop();
        return 1;
    }

    // Validate and apply configuration
    CameraConfiguration::Status configStatus = config->validate();
    if(configStatus == CameraConfiguration::Invalid) {
        cerr << "Invalid camera configuration." << endl;
        camera->release();
        manager.stop();
        return 1;
    }
    ret = camera->configure(config.get());
    if(ret < 0) {
        cerr << "Failed to configure camera." << endl;
        camera->release();
        manager.stop();
        return 1;
    }

    ret = camera->start();
    if(ret < 0) {
        cerr << "Failed to start camera." << endl;
        camera->release();
        manager.stop();
        return 1;
    }

    cout << "Camera started successfully." << endl;
    cout << "Capturing image every second..." << endl;

    /* // Allocate buffers for the stream using FrameBufferAllocator.
    //FrameBufferAllocator allocator(camera);
    std::unique_ptr<FrameBufferAllocator> allocator = std::make_unique<FrameBufferAllocator>(camera);
    Stream *stream = streamConfig.stream();

    ret = allocator.allocate(config.get());
    if(ret < 0) {
        cerr << "Failed to allocate buffers." << endl;
        camera->release();
        manager.stop();
        return 1;
    }

    // Get the stream from the configuration and check buffers.
    Stream *stream = config->at(0).stream();
    auto &buffers = allocator.buffers(stream);
    if(buffers.empty()) {
        cerr << "No buffers allocated for the stream." << endl;
        camera->release();
        manager.stop();
        return 1;
    } */

    //5. Create and allocate the request.
    std::unique_ptr<FrameBufferAllocator> allocator = std::make_unique<FrameBufferAllocator>(camera);
    //Stream *stream = streamConfig.stream();
    Stream *stream = config->at(0).stream();

    if (allocator->allocate(stream) < 0) {
        std::cerr << "Error: Failed to allocate capture buffers" << std::endl;
        return -1;
    }

    /* std::unique_ptr<Request> request = camera->createRequest();
    if (!request) {
        std::cerr << "Error: Failed to create request" << std::endl;
      return -1;
    }

    const std::unique_ptr<FrameBuffer> &buffer = allocator->buffers(stream)[0];
    if (request->addBuffer(stream, buffer.get()) < 0) {
        std::cerr << "Error: Failed to add buffer to request" << std::endl;
        return -1;
    } */

    // Main loop: Capture an image every second.
    while (true) {
        // Create a capture request.
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request) {
            cerr << "Failed to create capture request." << endl;
            break;
        }

        // Add the allocated buffer to the request.
        const std::unique_ptr<FrameBuffer> &buffer = allocator->buffers(stream)[0];
        if (request->addBuffer(stream, buffer.get()) < 0) {
            cerr << "Failed to add buffer to the capture request." << endl;
            break;
        }

        // Queue the request to the camera.
        ret = camera->queueRequest(request.get());
        if (ret < 0) {
            cerr << "Failed to queue capture request." << endl;
            break;
        }

        // Wait for the request to complete.
        // In a real implementation, a proper request completion callback should be used.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Access the captured image buffer.
        // The buffer should now be filled with image data by libcamera.
        unsigned char *rgbBuffer = reinterpret_cast<unsigned char*>(buffer->planes()[0].data);
        int width = 1920;
        int height = 1080;
        int channels = 3;

        // Compress the image using libjpeg-turbo.
        tjhandle tjInstance = tjInitCompress();
        if (!tjInstance) {
            cerr << "Failed to initialize TurboJPEG compressor." << endl;
            break;
        }

        unsigned char* jpegBuf = nullptr;
        unsigned long jpegSize = 0;
        int quality = 20;       // Low quality for smaller file size.
        int subsamp = TJSAMP_420; // Use 4:2:0 subsampling.

        int compStatus = tjCompress2(
            tjInstance,
            rgbBuffer,
            width,
            0, // Default pitch (width * channels)
            height,
            TJPF_RGB,
            &jpegBuf,
            &jpegSize,
            subsamp,
            quality,
            TJFLAG_FASTDCT
        );

        if (compStatus < 0) {
            cerr << "JPEG compression failed: " << tjGetErrorStr() << endl;
            tjDestroy(tjInstance);
            break;
        }

        // Generate filename based on current timestamp.
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        char filename[64];
        std::strftime(filename, sizeof(filename), "%Y%m%d_%H%M%S.jpg", std::localtime(&now_time));

        // Write the JPEG image to file.
        std::ofstream outFile(filename, std::ios::binary);
        if (!outFile) {
            cerr << "Failed to open file for writing: " << filename << endl;
        } else {
            outFile.write(reinterpret_cast<char*>(jpegBuf), jpegSize);
            outFile.close();
            cout << "Saved image to " << filename << endl;
        }

        tjFree(jpegBuf);
        tjDestroy(tjInstance);

        // Wait for 1 second before capturing the next image.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Stop the camera and clean up.
    camera->stop();
    camera->release();
    manager.stop();

    return 0;
}
