#ifndef COMPRESSION_UTILS_LZMA_UTILS_H
#define COMPRESSION_UTILS_LZMA_UTILS_H

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <memory>

namespace Compression {
namespace Utils {

class LZMAUtils {
public:
    struct LZMAConfig {
        int dictSize;
        int lc;
        int lp;
        int pb;
        int fb;
        int algo;
        
        LZMAConfig() : 
            dictSize(1 << 23),
            lc(3),
            lp(0),
            pb(2),
            fb(128),
            algo(1) {}
    };

    static bool compress(const std::vector<uint8_t>& input, 
                         std::vector<uint8_t>& output,
                         const LZMAConfig& config = LZMAConfig());

    static bool decompress(const std::vector<uint8_t>& input, 
                           std::vector<uint8_t>& output,
                           size_t expectedSize);

    static bool compressWithProps(const std::vector<uint8_t>& input,
                                   std::vector<uint8_t>& output,
                                   std::vector<uint8_t>& props,
                                   const LZMAConfig& config = LZMAConfig());

    static bool decompressWithProps(const std::vector<uint8_t>& input,
                                     const std::vector<uint8_t>& props,
                                     std::vector<uint8_t>& output,
                                     size_t expectedSize);

    static uint64_t calculateCRC64(const std::vector<uint8_t>& data);
    static uint64_t calculateCRC64(uint64_t crc, const std::vector<uint8_t>& data);
    static uint64_t getInitialCRC64();

    static void writeVarInt(std::vector<uint8_t>& output, uint64_t value);
    static uint64_t readVarInt(const std::vector<uint8_t>& input, size_t& offset);
    static uint64_t readVarInt(const uint8_t* input, size_t maxSize, size_t& offset);
    static size_t getVarIntSize(uint64_t value);

    static bool isLzmaAvailable();
};

}
}

#endif
