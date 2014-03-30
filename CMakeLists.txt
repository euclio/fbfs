cmake_minimum_required(VERSION 2.6)
project(fbfs)
add_executable(fbfs fbfs.cpp)
add_definitions(
    -std=c++11
    -D_FILE_OFFSET_BITS=64
    -I/usr/include/fuse
    -lfuse
    -pthread
)

add_subdirectory(third_party/curlcpp)
include_directories(${CURLCPP_SOURCE_DIR}/include)

add_subdirectory(third_party/JsonBox)
include_directories(${JSONBOX_SOURCE_DIR}/include)
