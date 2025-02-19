# mkdir -p build
# cd build
# cmake ..
# make -j$(nproc)

cmake_minimum_required(VERSION 3.10)
project(camera_capture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Packages
set(ncnn_DIR "/home/pi/ncnn/build/install/lib/cmake/ncnn")
find_package(PkgConfig)
find_package(ncnn REQUIRED)
pkg_check_modules(TURBOJPEG REQUIRED IMPORTED_TARGET libturbojpeg)
pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
pkg_check_modules(OPENCV REQUIRED IMPORTED_TARGET opencv4)
pkg_check_modules(LIBEVENT REQUIRED IMPORTED_TARGET libevent_pthreads)

#For good measure, we will include the directories as well /usr/include/libcamera
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${LIBCAMERA_INCLUDE_DIRS} 
    ${LIBEVENT_INCLUDE_DIRS}
    ${ncnn_INCLUDE_DIRS}
    ${OPENCV_INCLUDE_DIRS}
    ${TURBOJPEG_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME} camera_capture_v2.cpp save_jpeg.cpp event_loop.cpp ncnn_inference.cpp)

target_link_libraries(${PROJECT_NAME} ncnn)
target_link_libraries(${PROJECT_NAME} PkgConfig::TURBOJPEG)
target_link_libraries(${PROJECT_NAME} PkgConfig::LIBCAMERA)
target_link_libraries(${PROJECT_NAME} PkgConfig::OPENCV)
target_link_libraries(${PROJECT_NAME} PkgConfig::LIBEVENT)

