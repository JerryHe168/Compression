#ifndef COMPRESSION_CORE_7Z_SZCOMPRESSOR_H
#define COMPRESSION_CORE_7Z_SZCOMPRESSOR_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <memory>
#include <map>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

#include "compression/core/7z/constants.h"
#include "compression/utils/lzma_utils.h"

namespace Compression {
namespace Core {
namespace SevenZip {

struct SevenZipFileInfo {
    std::string name;
    uint64_t size;
    uint64_t packedSize;
    uint64_t crc64;
    uint32_t attributes;
    uint64_t modificationTime;
    bool isDirectory;
    size_t dataOffset;
    std::vector<uint8_t> lzmaProps;
};

class SzCompressor {
public:
    SzCompressor();
    ~SzCompressor();

    bool open(const std::string& szPath);
    bool addFile(const std::string& filePath, const std::string& entryName = "");
    bool addDirectory(const std::string& dirPath, const std::string& basePath = "");
    bool close();

    void setCompressionLevel(int level);
    int getCompressionLevel() const;

private:
    std::ofstream szFile;
    std::vector<SevenZipFileInfo> fileInfos;
    std::vector<uint8_t> compressedData;
    int compressionLevel;

    bool writeSignatureHeader();
    bool writeHeader();
    bool writeStreamsInfoForHeader(std::vector<uint8_t>& output);
    bool writeFilesInfoForHeader(std::vector<uint8_t>& output);

    bool compressAndAddFile(const std::string& filePath, const std::string& entryName);
    std::vector<uint8_t> readFileContent(const std::string& filePath);
    bool isDirectory(const std::string& path);
    void listFilesInDirectory(const std::string& dirPath, std::vector<std::string>& files);
    std::string getRelativePath(const std::string& basePath, const std::string& fullPath);
};

}
}
}

#endif
