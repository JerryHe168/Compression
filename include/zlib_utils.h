#ifndef ZLIB_UTILS_H
#define ZLIB_UTILS_H

#include <vector>
#include <cstdint>
#include <stdexcept>

namespace Compression {

class ZlibUtils {
public:
    static bool compress(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
    static bool decompress(const std::vector<uint8_t>& input, std::vector<uint8_t>& output, size_t expectedSize);
    static uint32_t calculateCRC32(const std::vector<uint8_t>& data);
    static uint32_t calculateCRC32(uint32_t crc, const std::vector<uint8_t>& data);
    static uint32_t getInitialCRC();
};

} // namespace Compression

#endif // ZLIB_UTILS_H
