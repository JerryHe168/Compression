#ifndef ZIPPER_H
#define ZIPPER_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

namespace Compression {

class Zipper {
public:
    Zipper();
    ~Zipper();

    bool open(const std::string& zipPath);
    bool addFile(const std::string& filePath, const std::string& entryName = "");
    bool addDirectory(const std::string& dirPath, const std::string& basePath = "");
    bool close();

private:
    std::ofstream zipFile;
    std::vector<uint8_t> centralDirectory;
    uint32_t centralDirectorySize;
    uint32_t centralDirectoryOffset;
    uint16_t entryCount;

    bool writeLocalFileHeader(const std::string& entryName, uint32_t uncompressedSize,
                            uint32_t compressedSize, uint32_t crc32,
                            uint16_t compressionMethod, uint32_t& localHeaderOffset);
    bool writeDataDescriptor(uint32_t crc32, uint32_t compressedSize,
                            uint32_t uncompressedSize);
    bool writeCentralDirectoryHeader(const std::string& entryName, uint32_t uncompressedSize,
                                    uint32_t compressedSize, uint32_t crc32,
                                    uint16_t compressionMethod, uint32_t localHeaderOffset);
    bool writeEndOfCentralDirectory();
    bool compressFile(const std::string& filePath, std::vector<uint8_t>& compressedData,
                      uint32_t& uncompressedSize, uint32_t& crc32);
    uint32_t calculateCRC32(const std::vector<uint8_t>& data);
    std::string getRelativePath(const std::string& basePath, const std::string& fullPath);
    bool isDirectory(const std::string& path);
    void listFilesInDirectory(const std::string& dirPath, std::vector<std::string>& files);
};

} // namespace Compression

#endif // ZIPPER_H
