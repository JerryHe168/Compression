cmake_minimum_required(VERSION 3.10)
project(CompressionLib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

message(STATUS "========================================")
message(STATUS "  Compression Library Build")
message(STATUS "========================================")

set(ZLIB_SRC_DIR ${CMAKE_SOURCE_DIR}/third_party/zlib/src)
set(LZMA_BASE_DIR ${CMAKE_SOURCE_DIR}/third_party/liblzma)

set(ZLIB_SOURCES
    ${ZLIB_SRC_DIR}/adler32.c
    ${ZLIB_SRC_DIR}/compress.c
    ${ZLIB_SRC_DIR}/crc32.c
    ${ZLIB_SRC_DIR}/deflate.c
    ${ZLIB_SRC_DIR}/gzclose.c
    ${ZLIB_SRC_DIR}/gzlib.c
    ${ZLIB_SRC_DIR}/gzread.c
    ${ZLIB_SRC_DIR}/gzwrite.c
    ${ZLIB_SRC_DIR}/infback.c
    ${ZLIB_SRC_DIR}/inffast.c
    ${ZLIB_SRC_DIR}/inflate.c
    ${ZLIB_SRC_DIR}/inftrees.c
    ${ZLIB_SRC_DIR}/trees.c
    ${ZLIB_SRC_DIR}/uncompr.c
    ${ZLIB_SRC_DIR}/zutil.c
)

set(LZMA_SOURCES
    ${LZMA_BASE_DIR}/src/liblzma/check/check.c
    ${LZMA_BASE_DIR}/src/liblzma/check/crc32_fast.c
    ${LZMA_BASE_DIR}/src/liblzma/check/crc64_fast.c
    ${LZMA_BASE_DIR}/src/liblzma/check/sha256.c
    ${LZMA_BASE_DIR}/src/liblzma/common/alone_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/alone_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/auto_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/block_buffer_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/block_buffer_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/block_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/block_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/block_header_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/block_header_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/block_util.c
    ${LZMA_BASE_DIR}/src/liblzma/common/common.c
    ${LZMA_BASE_DIR}/src/liblzma/common/easy_buffer_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/easy_decoder_memusage.c
    ${LZMA_BASE_DIR}/src/liblzma/common/easy_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/easy_encoder_memusage.c
    ${LZMA_BASE_DIR}/src/liblzma/common/easy_preset.c
    ${LZMA_BASE_DIR}/src/liblzma/common/file_info.c
    ${LZMA_BASE_DIR}/src/liblzma/common/filter_buffer_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/filter_buffer_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/filter_common.c
    ${LZMA_BASE_DIR}/src/liblzma/common/filter_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/filter_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/filter_flags_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/filter_flags_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/hardware_cputhreads.c
    ${LZMA_BASE_DIR}/src/liblzma/common/hardware_physmem.c
    ${LZMA_BASE_DIR}/src/liblzma/common/index.c
    ${LZMA_BASE_DIR}/src/liblzma/common/index_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/index_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/index_hash.c
    ${LZMA_BASE_DIR}/src/liblzma/common/lzip_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/microlzma_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/microlzma_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/outqueue.c
    ${LZMA_BASE_DIR}/src/liblzma/common/stream_buffer_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/stream_buffer_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/stream_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/stream_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/stream_flags_common.c
    ${LZMA_BASE_DIR}/src/liblzma/common/stream_flags_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/stream_flags_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/string_conversion.c
    ${LZMA_BASE_DIR}/src/liblzma/common/vli_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/vli_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/common/vli_size.c
    ${LZMA_BASE_DIR}/src/liblzma/lz/lz_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/lz/lz_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/lz/lz_encoder_mf.c
    ${LZMA_BASE_DIR}/src/liblzma/lzma/fastpos_table.c
    ${LZMA_BASE_DIR}/src/liblzma/lzma/lzma2_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/lzma/lzma2_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/lzma/lzma_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/lzma/lzma_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/lzma/lzma_encoder_optimum_fast.c
    ${LZMA_BASE_DIR}/src/liblzma/lzma/lzma_encoder_optimum_normal.c
    ${LZMA_BASE_DIR}/src/liblzma/lzma/lzma_encoder_presets.c
    ${LZMA_BASE_DIR}/src/liblzma/rangecoder/price_table.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/arm.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/arm64.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/armthumb.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/ia64.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/powerpc.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/riscv.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/simple_coder.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/simple_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/simple_encoder.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/sparc.c
    ${LZMA_BASE_DIR}/src/liblzma/simple/x86.c
    ${LZMA_BASE_DIR}/src/liblzma/delta/delta_common.c
    ${LZMA_BASE_DIR}/src/liblzma/delta/delta_decoder.c
    ${LZMA_BASE_DIR}/src/liblzma/delta/delta_encoder.c
)

message(STATUS "Building bundled zlib from: ${ZLIB_SRC_DIR}")
add_library(zlib_bundled STATIC ${ZLIB_SOURCES})
target_include_directories(zlib_bundled PUBLIC ${ZLIB_SRC_DIR})
add_definitions(-DHAVE_ZLIB)

message(STATUS "Building bundled liblzma from: ${LZMA_BASE_DIR}")
add_library(liblzma_bundled STATIC ${LZMA_SOURCES})

target_include_directories(liblzma_bundled PUBLIC
    ${LZMA_BASE_DIR}
    ${LZMA_BASE_DIR}/src/common
    ${LZMA_BASE_DIR}/src/liblzma
    ${LZMA_BASE_DIR}/src/liblzma/api
    ${LZMA_BASE_DIR}/src/liblzma/common
    ${LZMA_BASE_DIR}/src/liblzma/check
    ${LZMA_BASE_DIR}/src/liblzma/lz
    ${LZMA_BASE_DIR}/src/liblzma/lzma
    ${LZMA_BASE_DIR}/src/liblzma/rangecoder
    ${LZMA_BASE_DIR}/src/liblzma/simple
    ${LZMA_BASE_DIR}/src/liblzma/delta
)

target_compile_definitions(liblzma_bundled PRIVATE
    HAVE_CONFIG_H
    _GNU_SOURCE
)

if(WIN32)
    target_compile_definitions(liblzma_bundled PRIVATE
        _WIN32
    )
endif()

add_definitions(-DHAVE_LZMA)

add_library(compression STATIC
    src/core/zip/zipper.cpp
    src/core/zip/unzipper.cpp
    src/utils/zlib_utils.cpp
    src/core/7z/szcompressor.cpp
    src/core/7z/szdecompressor.cpp
    src/utils/lzma_utils.cpp
)

target_compile_definitions(compression PRIVATE
    LZMA_API_STATIC
)

target_include_directories(compression PUBLIC
    ${ZLIB_SRC_DIR}
    ${LZMA_BASE_DIR}/src/liblzma/api
)

target_link_libraries(compression
    zlib_bundled
    liblzma_bundled
)

add_executable(example examples/main.cpp)
target_link_libraries(example compression)

message(STATUS "========================================")
message(STATUS "  Configuration Complete")
message(STATUS "========================================")
message(STATUS "ZLIB:   Bundled (${ZLIB_SRC_DIR})")
message(STATUS "LibLZMA: Bundled (${LZMA_BASE_DIR})")
message(STATUS "========================================")
