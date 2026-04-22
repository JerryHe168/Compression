#ifndef UNZIPPER_H
#define UNZIPPER_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <map>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

namespace Compression {

namespace ZipLimits {
    constexpr uint32_t MAX_UNCOMPRESSED_SIZE = 100 * 1024 * 1024;
    constexpr uint32_t MAX_COMPRESSED_SIZE = 100 * 1024 * 1024;
    constexpr uint16_t MAX_ENTRY_COUNT = 10000;
    constexpr uint16_t MAX_COMMENT_SIZE = 65535;
    constexpr uint16_t MAX_FILENAME_LENGTH = 65535;
}

struct ZipEntryInfo {
    std::string name;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint32_t crc32;
    uint32_t localHeaderOffset;
    uint16_t compressionMethod;
    bool isDirectory;
};

class Unzipper {
public:
    Unzipper();
    ~Unzipper();

    bool open(const std::string& zipPath);
    bool extractTo(const std::string& outputDir);
    bool extractFile(const std::string& entryName, const std::string& outputPath);
    std::vector<std::string> listEntries();
    ZipEntryInfo getEntryInfo(const std::string& entryName);
    bool close();

private:
    std::ifstream zipFile;
    std::map<std::string, ZipEntryInfo> entries;

    bool readCentralDirectory();
    bool readEndOfCentralDirectory(uint32_t& centralDirectoryOffset, uint16_t& entryCount);
    bool readLocalFileHeader(uint32_t localHeaderOffset,
                            uint32_t& dataOffset,
                            uint32_t& compressedSize,
                            uint16_t& compressionMethod);
    bool createDirectory(const std::string& path);
    bool writeFile(const std::string& filePath, const std::vector<uint8_t>& data);
    bool entryExists(const std::string& entryName);
    bool isSafeEntryName(const std::string& entryName);
    std::string joinPath(const std::string& base, const std::string& name);
};

} // namespace Compression

#endif // UNZIPPER_H
