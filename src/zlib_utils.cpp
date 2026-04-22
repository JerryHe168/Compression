#include "zlib_utils.h"
#include <zlib.h>
#include <cstring>

namespace Compression {

bool ZlibUtils::compress(const std::vector<uint8_t>& input, std::vector<uint8_t>& output)
{
    z_stream deflateStream;
    std::memset(&deflateStream, 0, sizeof(deflateStream));

    if (deflateInit(&deflateStream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        return false;
    }

    deflateStream.next_in = const_cast<uint8_t*>(input.data());
    deflateStream.avail_in = static_cast<uInt>(input.size());

    size_t bufferSize = deflateBound(&deflateStream, static_cast<uLong>(input.size()));
    output.resize(bufferSize);

    deflateStream.next_out = output.data();
    deflateStream.avail_out = static_cast<uInt>(output.size());

    int deflateResult = deflate(&deflateStream, Z_FINISH);
    if (deflateResult != Z_STREAM_END) {
        deflateEnd(&deflateStream);
        return false;
    }

    output.resize(deflateStream.total_out);
    deflateEnd(&deflateStream);

    return true;
}

bool ZlibUtils::decompress(const std::vector<uint8_t>& input, std::vector<uint8_t>& output, size_t expectedSize)
{
    z_stream inflateStream;
    std::memset(&inflateStream, 0, sizeof(inflateStream));

    if (inflateInit(&inflateStream) != Z_OK) {
        return false;
    }

    inflateStream.next_in = const_cast<uint8_t*>(input.data());
    inflateStream.avail_in = static_cast<uInt>(input.size());

    output.resize(expectedSize);

    inflateStream.next_out = output.data();
    inflateStream.avail_out = static_cast<uInt>(output.size());

    int inflateResult = inflate(&inflateStream, Z_FINISH);
    if (inflateResult != Z_STREAM_END && inflateResult != Z_OK) {
        inflateEnd(&inflateStream);
        return false;
    }

    output.resize(inflateStream.total_out);
    inflateEnd(&inflateStream);

    return true;
}

uint32_t ZlibUtils::calculateCRC32(const std::vector<uint8_t>& data)
{
    return crc32(0L, Z_NULL, 0) ^ crc32(crc32(0L, Z_NULL, 0), data.data(), static_cast<uInt>(data.size()));
}

uint32_t ZlibUtils::calculateCRC32(uint32_t crc, const std::vector<uint8_t>& data)
{
    return crc32(crc, data.data(), static_cast<uInt>(data.size()));
}

uint32_t ZlibUtils::getInitialCRC()
{
    return crc32(0L, Z_NULL, 0);
}

} // namespace Compression
