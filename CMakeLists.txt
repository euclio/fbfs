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
