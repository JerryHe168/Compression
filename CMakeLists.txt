cmake_minimum_required(VERSION 3.10)
project(CompressionLib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(ZLIB QUIET)
find_package(LibLZMA QUIET)

if(ZLIB_FOUND)
    message(STATUS "Found ZLIB: ${ZLIB_LIBRARIES}")
    add_definitions(-DHAVE_ZLIB)
else()
    message(STATUS "ZLIB not found. ZIP compression will use storage-only mode.")
    message(STATUS "Install ZLIB to enable DEFLATE compression for ZIP.")
endif()

if(LIBLZMA_FOUND)
    message(STATUS "Found LibLZMA: ${LIBLZMA_LIBRARIES}")
    add_definitions(-DHAVE_LZMA)
else()
    message(STATUS "LibLZMA not found. 7z compression will use storage-only mode.")
    message(STATUS "Install LibLZMA (xz-utils) to enable LZMA/LZMA2 compression for 7z.")
endif()

add_library(compression STATIC
    src/core/zip/zipper.cpp
    src/core/zip/unzipper.cpp
    src/utils/zlib_utils.cpp
    src/core/7z/szcompressor.cpp
    src/core/7z/szdecompressor.cpp
    src/utils/lzma_utils.cpp
)

if(ZLIB_FOUND)
    target_link_libraries(compression ZLIB::ZLIB)
endif()

if(LIBLZMA_FOUND)
    target_link_libraries(compression LibLZMA::LibLZMA)
endif()

add_executable(example examples/main.cpp)
target_link_libraries(example compression)
