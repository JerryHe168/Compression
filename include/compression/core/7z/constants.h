#ifndef COMPRESSION_CORE_7Z_CONSTANTS_H
#define COMPRESSION_CORE_7Z_CONSTANTS_H

#include <cstdint>

namespace Compression {
namespace Core {
namespace SevenZip {

namespace SevenZipLimits {
    constexpr uint64_t MAX_UNCOMPRESSED_SIZE = 100ULL * 1024 * 1024 * 1024;
    constexpr uint64_t MAX_COMPRESSED_SIZE = 100ULL * 1024 * 1024 * 1024;
    constexpr uint32_t MAX_ENTRY_COUNT = 10000;
    constexpr uint32_t MAX_FILENAME_LENGTH = 32767;
    constexpr uint32_t MAX_COMPRESSION_RATIO = 1000;
    constexpr uint64_t MIN_COMPRESSED_SIZE_FOR_RATIO_CHECK = 1024;
    constexpr uint64_t MAX_SINGLE_FILE_SIZE = 4ULL * 1024 * 1024 * 1024;
}

namespace SevenZipSignatures {
    constexpr uint64_t SIGNATURE_7Z = 0x7AFBC273;
    constexpr uint8_t SIGNATURE_BYTES[6] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};
}

namespace SevenZipVersions {
    constexpr uint8_t MAJOR_VERSION = 0;
    constexpr uint8_t MINOR_VERSION = 4;
}

namespace SevenZipMethods {
    constexpr uint8_t METHOD_LZMA = 0x03;
    constexpr uint8_t METHOD_LZMA2 = 0x21;
    constexpr uint8_t METHOD_PPMD = 0x04;
    constexpr uint8_t METHOD_COPY = 0x00;
    constexpr uint8_t METHOD_DEFLATE = 0x08;
    constexpr uint8_t METHOD_BZIP2 = 0x02;
}

namespace SevenZipIDs {
    constexpr uint8_t ID_END = 0x00;
    constexpr uint8_t ID_HEADER = 0x01;
    constexpr uint8_t ID_ARCHIVE_PROPERTIES = 0x02;
    constexpr uint8_t ID_ADDITIONAL_STREAMS_INFO = 0x03;
    constexpr uint8_t ID_MAIN_STREAMS_INFO = 0x04;
    constexpr uint8_t ID_FILES_INFO = 0x05;
    constexpr uint8_t ID_PACK_INFO = 0x06;
    constexpr uint8_t ID_UNPACK_INFO = 0x07;
    constexpr uint8_t ID_SUBSTREAMS_INFO = 0x08;
    constexpr uint8_t ID_SIZE = 0x09;
    constexpr uint8_t ID_CRC = 0x0A;
    constexpr uint8_t ID_FOLDER = 0x0B;
    constexpr uint8_t ID_CODERS_UNPACK_SIZE = 0x0C;
    constexpr uint8_t ID_NUM_UNPACK_STREAM = 0x0D;
    constexpr uint8_t ID_EMPTY_STREAM = 0x0E;
    constexpr uint8_t ID_EMPTY_FILE = 0x0F;
    constexpr uint8_t ID_ANTI = 0x10;
    constexpr uint8_t ID_NAME = 0x11;
    constexpr uint8_t ID_CREATION_TIME = 0x12;
    constexpr uint8_t ID_ACCESS_TIME = 0x13;
    constexpr uint8_t ID_MODIFICATION_TIME = 0x14;
    constexpr uint8_t ID_WIN_ATTRIBUTES = 0x15;
    constexpr uint8_t ID_COMMENT = 0x16;
    constexpr uint8_t ID_ENCODED_HEADER = 0x17;
    constexpr uint8_t ID_START_POS = 0x18;
    constexpr uint8_t ID_DUMMY = 0x19;
}

namespace LZMAConstants {
    constexpr uint32_t LZMA_PROPS_SIZE = 5;
    constexpr uint32_t LZMA_DIC_MIN = (1 << 12);
    constexpr uint32_t LZMA_DIC_DEFAULT = (1 << 23);
    constexpr uint32_t LZMA_DIC_MAX = (1 << 30);
    constexpr int LZMA_LC_DEFAULT = 3;
    constexpr int LZMA_LP_DEFAULT = 0;
    constexpr int LZMA_PB_DEFAULT = 2;
    constexpr int LZMA_FB_DEFAULT = 128;
}

}
}
}

#endif
