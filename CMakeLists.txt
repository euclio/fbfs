cmake_minimum_required(VERSION 2.6)

project(fbfs)

add_definitions(
    -std=c++11
    -g
    -Wall
)

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h)
include_directories("include")

add_executable(fbfs ${SOURCES} ${HEADERS})

# Configure FUSE
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/FindFUSE.cmake")
find_package(FUSE 2.6 REQUIRED)
add_definitions(${FUSE_DEFINITIONS})
include_directories(${FUSE_INCLUDE_DIRS})

add_subdirectory(third_party/curlcpp)
include_directories(${CURLCPP_SOURCE_DIR}/include)

add_subdirectory(third_party/JsonBox)
include_directories(${JsonBox_SOURCE_DIR}/include)
target_link_libraries(fbfs ${FUSE_LIBRARIES} JsonBox curlcpp)
