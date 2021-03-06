cmake_minimum_required(VERSION 2.8.8)

project(fbfs)

add_definitions(
    -std=c++11
    -g
    -Wall
)

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h)
include_directories("include")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Boost COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Qt5WebKit REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5WebKitWidgets REQUIRED)

# Configure FUSE
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/FindFUSE.cmake")
find_package(FUSE 2.6 REQUIRED)
add_definitions(${FUSE_DEFINITIONS})
include_directories(${FUSE_INCLUDE_DIRS})

add_subdirectory(third_party/curlcpp)
include_directories(${CURLCPP_SOURCE_DIR}/include)

add_subdirectory(third_party/json-spirit)
include_directories(${json_spirit_SOURCE_DIR}/json_spirit)

add_executable(fbfs ${SOURCES} ${HEADERS})
target_link_libraries(fbfs
    ${FUSE_LIBRARIES}
    json_spirit
    curlcpp
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)
qt5_use_modules(fbfs WebKit Widgets WebKitWidgets)
set_target_properties(fbfs PROPERTIES AUTOMOC TRUE)
