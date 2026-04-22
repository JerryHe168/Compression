#ifndef ZIP_CONSTANTS_H
#define ZIP_CONSTANTS_H

#include <cstdint>

namespace Compression {

namespace ZipLimits {
    constexpr uint32_t MAX_UNCOMPRESSED_SIZE = 100 * 1024 * 1024;
    constexpr uint32_t MAX_COMPRESSED_SIZE = 100 * 1024 * 1024;
    constexpr uint16_t MAX_ENTRY_COUNT = 10000;
    constexpr uint16_t MAX_COMMENT_SIZE = 65535;
    constexpr uint16_t MAX_FILENAME_LENGTH = 65535;
    constexpr uint32_t MAX_COMPRESSION_RATIO = 100;
    constexpr uint32_t MIN_COMPRESSED_SIZE_FOR_RATIO_CHECK = 1024;
}

namespace ZipSignatures {
    constexpr uint32_t LOCAL_FILE_HEADER = 0x04034b50;
    constexpr uint32_t CENTRAL_FILE_HEADER = 0x02014b50;
    constexpr uint32_t END_OF_CENTRAL_DIR = 0x06054b50;
}

namespace ZipVersions {
    constexpr uint16_t VERSION_NEEDED = 20;
}

namespace ZipMethods {
    constexpr uint16_t STORED = 0;
    constexpr uint16_t DEFLATED = 8;
}

}

#endif
