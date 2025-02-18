cmake_minimum_required(VERSION 3.10)
project(camera_capture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find NCNN
set(ncnn_DIR "/home/pi/ncnn/build/install/lib/cmake/ncnn")
find_package(ncnn REQUIRED)
if(ncnn_FOUND)
    message(STATUS "ncnn found: ${ncnn_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "ncnn not found")
endif()


add_executable(${PROJECT_NAME} camera_capture.cpp ncnn_inference.cpp)

target_link_libraries(${PROJECT_NAME}
    camera
    camera-base
    turbojpeg
    ncnn
    #${ncnn_LIBRARIES}
)

#For good measure, we will include the directories as well
target_include_directories(${PROJECT_NAME} PUBLIC
    /usr/include/libcamera
    ${ncnn_INCLUDE_DIRS} #${ncnn_INCLUDE_DIRS} #/home/pi/ncnn/build/install/include
)
