cmake_minimum_required(VERSION 3.10)
project(CompressionLib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(ZLIB REQUIRED)

add_library(compression STATIC
    src/zipper.cpp
    src/unzipper.cpp
    src/zlib_utils.cpp
)

target_link_libraries(compression ZLIB::ZLIB)

add_executable(example examples/main.cpp)
target_link_libraries(example compression)
