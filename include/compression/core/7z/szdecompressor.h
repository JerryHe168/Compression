#ifndef COMPRESSION_CORE_7Z_SZDECOMPRESSOR_H
#define COMPRESSION_CORE_7Z_SZDECOMPRESSOR_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <map>
#include <memory>

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

struct SevenZipEntry {
    std::string name;
    uint64_t size;
    uint64_t packedSize;
    uint64_t crc64;
    uint64_t localHeaderOffset;
    uint64_t dataOffset;
    bool isDirectory;
    std::vector<uint8_t> lzmaProps;
    uint8_t compressionMethod;
};

class SzDecompressor {
public:
    SzDecompressor();
    ~SzDecompressor();

    bool open(const std::string& szPath);
    bool extractTo(const std::string& outputDir);
    bool extractFile(const std::string& entryName, const std::string& outputPath);
    std::vector<std::string> listEntries();
    SevenZipEntry getEntryInfo(const std::string& entryName);
    bool close();

private:
    std::ifstream szFile;
    std::map<std::string, SevenZipEntry> entries;
    uint64_t headerOffset;
    uint64_t dataStartOffset;

    bool readSignatureHeader();
    bool readHeader();
    bool readPackInfo();
    bool readUnpackInfo();
    bool readStreamsInfo();
    bool readFilesInfo();
    bool readEncodedHeader();
    bool readEndHeader();

    uint8_t readID();
    uint64_t readVarInt();
    std::vector<uint8_t> readBytes(size_t count);
    std::string readString();

    bool createDirectory(const std::string& path);
    bool writeFile(const std::string& filePath, const std::vector<uint8_t>& data);
    bool entryExists(const std::string& entryName);
    bool isSafeEntryName(const std::string& entryName);
    std::string joinPath(const std::string& base, const std::string& name);

    bool decompressEntry(const SevenZipEntry& entry, std::vector<uint8_t>& output);
};

}
}
}

#endif
