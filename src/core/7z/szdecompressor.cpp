#include "compression/core/7z/szdecompressor.h"
#include <cstring>
#include <algorithm>

namespace Compression {
namespace Core {
namespace SevenZip {

SzDecompressor::SzDecompressor()
    : headerOffset(0)
    , dataStartOffset(0)
{
}

SzDecompressor::~SzDecompressor()
{
    close();
}

bool SzDecompressor::isSafeEntryName(const std::string& entryName)
{
    if (entryName.empty()) {
        return true;
    }

    if (entryName.length() >= 2 && entryName[0] == '.' && entryName[1] == '.') {
        return false;
    }

    size_t pos = 0;
    while ((pos = entryName.find("/", pos)) != std::string::npos) {
        if (pos + 2 < entryName.length() && 
            entryName[pos + 1] == '.' && entryName[pos + 2] == '.') {
            if (pos + 3 == entryName.length() || entryName[pos + 3] == '/') {
                return false;
            }
        }
        pos++;
    }

    if (entryName.find('\\') != std::string::npos) {
        return false;
    }

    if (entryName.find(":") != std::string::npos) {
        return false;
    }

    if (entryName.length() >= 2 && entryName[1] == ':') {
        return false;
    }

    return true;
}

std::string SzDecompressor::joinPath(const std::string& base, const std::string& name)
{
#ifdef _WIN32
    return base + "\\" + name;
#else
    return base + "/" + name;
#endif
}

bool SzDecompressor::open(const std::string& szPath)
{
    szFile.open(szPath, std::ios::binary);
    if (!szFile.is_open()) {
        throw std::runtime_error("Cannot open 7z file for reading: " + szPath);
    }

    entries.clear();

    if (!readSignatureHeader()) {
        close();
        return false;
    }

    if (!readHeader()) {
        close();
        return false;
    }

    return true;
}

uint8_t SzDecompressor::readID()
{
    uint8_t id;
    szFile.read(reinterpret_cast<char*>(&id), 1);
    if (!szFile.good()) {
        throw std::runtime_error("Cannot read ID from 7z file");
    }
    return id;
}

uint64_t SzDecompressor::readVarInt()
{
    uint64_t value = 0;
    int shift = 0;
    while (shift < 63) {
        uint8_t b;
        szFile.read(reinterpret_cast<char*>(&b), 1);
        if (!szFile.good()) {
            throw std::runtime_error("Cannot read VarInt from 7z file");
        }
        value |= static_cast<uint64_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0) {
            return value;
        }
        shift += 7;
    }
    return value;
}

std::vector<uint8_t> SzDecompressor::readBytes(size_t count)
{
    std::vector<uint8_t> data(count);
    szFile.read(reinterpret_cast<char*>(data.data()), count);
    if (!szFile.good()) {
        throw std::runtime_error("Cannot read bytes from 7z file");
    }
    return data;
}

std::string SzDecompressor::readString()
{
    std::string result;
    char c;
    while (true) {
        szFile.read(&c, 1);
        if (!szFile.good() || c == 0) {
            break;
        }
        result += c;
    }
    return result;
}

bool SzDecompressor::readSignatureHeader()
{
    uint8_t signature[6];
    szFile.read(reinterpret_cast<char*>(signature), 6);
    
    uint8_t expectedSignature[6] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};
    if (std::memcmp(signature, expectedSignature, 6) != 0) {
        throw std::runtime_error("Invalid 7z file signature");
    }

    uint8_t majorVersion, minorVersion;
    szFile.read(reinterpret_cast<char*>(&majorVersion), 1);
    szFile.read(reinterpret_cast<char*>(&minorVersion), 1);

    uint32_t startHeaderCRC;
    szFile.read(reinterpret_cast<char*>(&startHeaderCRC), 4);

    szFile.read(reinterpret_cast<char*>(&headerOffset), 8);

    uint64_t nextHeaderSize;
    szFile.read(reinterpret_cast<char*>(&nextHeaderSize), 8);

    uint32_t nextHeaderCRC;
    szFile.read(reinterpret_cast<char*>(&nextHeaderCRC), 4);

    dataStartOffset = static_cast<uint64_t>(szFile.tellg());

    return true;
}

bool SzDecompressor::readHeader()
{
    if (headerOffset == 0) {
        return true;
    }

    szFile.seekg(static_cast<std::streamoff>(headerOffset), std::ios::beg);
    if (!szFile.good()) {
        throw std::runtime_error("Cannot seek to header in 7z file");
    }

    uint64_t numFiles = 0;
    std::vector<std::string> names;
    std::vector<uint64_t> sizes;
    std::vector<uint64_t> crcs;
    std::vector<uint64_t> packedSizes;

    while (true) {
        uint8_t id = readID();
        
        if (id == SevenZipIDs::ID_END) {
            break;
        }

        switch (id) {
            case SevenZipIDs::ID_FILES_INFO: {
                numFiles = readVarInt();
                if (numFiles > SevenZipLimits::MAX_ENTRY_COUNT) {
                    throw std::runtime_error("Entry count exceeds maximum allowed in 7z file");
                }
                break;
            }
            case SevenZipIDs::ID_NAME: {
                for (uint64_t i = 0; i < numFiles; ++i) {
                    names.push_back(readString());
                }
                break;
            }
            case SevenZipIDs::ID_SIZE: {
                for (uint64_t i = 0; i < numFiles; ++i) {
                    uint64_t size = readVarInt();
                    if (size > SevenZipLimits::MAX_UNCOMPRESSED_SIZE) {
                        throw std::runtime_error("Uncompressed size exceeds maximum allowed in 7z file");
                    }
                    sizes.push_back(size);
                }
                break;
            }
            case SevenZipIDs::ID_CRC: {
                for (uint64_t i = 0; i < numFiles; ++i) {
                    uint64_t crc;
                    szFile.read(reinterpret_cast<char*>(&crc), 8);
                    crcs.push_back(crc);
                }
                break;
            }
            case SevenZipIDs::ID_PACK_INFO: {
                uint64_t numPackStreams = readVarInt();
                for (uint64_t i = 0; i < numPackStreams; ++i) {
                    uint64_t packSize = readVarInt();
                    if (packSize > SevenZipLimits::MAX_COMPRESSED_SIZE) {
                        throw std::runtime_error("Compressed size exceeds maximum allowed in 7z file");
                    }
                    packedSizes.push_back(packSize);
                }
                break;
            }
            case SevenZipIDs::ID_SUBSTREAMS_INFO: {
                uint64_t numSubStreams = readVarInt();
                for (uint64_t i = 0; i < numSubStreams; ++i) {
                    uint64_t unpackSize = readVarInt();
                    if (unpackSize > SevenZipLimits::MAX_UNCOMPRESSED_SIZE) {
                        throw std::runtime_error("Uncompressed size exceeds maximum allowed in 7z file");
                    }
                }
                break;
            }
            default: {
                continue;
            }
        }
    }

    for (size_t i = 0; i < names.size() && i < numFiles; ++i) {
        SevenZipEntry entry;
        entry.name = names[i];
        entry.size = (i < sizes.size()) ? sizes[i] : 0;
        entry.packedSize = (i < packedSizes.size()) ? packedSizes[i] : entry.size;
        entry.crc64 = (i < crcs.size()) ? crcs[i] : 0;
        entry.isDirectory = (!entry.name.empty() && entry.name.back() == '/');
        entry.dataOffset = dataStartOffset;
        entry.compressionMethod = SevenZipMethods::METHOD_LZMA2;
        entry.localHeaderOffset = 0;

        if (entry.size > SevenZipLimits::MIN_COMPRESSED_SIZE_FOR_RATIO_CHECK &&
            entry.packedSize > 0) {
            uint64_t ratio = entry.size / entry.packedSize;
            if (ratio > SevenZipLimits::MAX_COMPRESSION_RATIO) {
                throw std::runtime_error("Suspicious compression ratio detected - possible 7z bomb");
            }
        }

        entries[entry.name] = entry;
    }

    return true;
}

std::vector<std::string> SzDecompressor::listEntries()
{
    std::vector<std::string> names;
    for (const auto& pair : entries) {
        names.push_back(pair.first);
    }
    return names;
}

SevenZipEntry SzDecompressor::getEntryInfo(const std::string& entryName)
{
    if (!entryExists(entryName)) {
        throw std::runtime_error("Entry not found: " + entryName);
    }
    return entries[entryName];
}

bool SzDecompressor::entryExists(const std::string& entryName)
{
    return entries.find(entryName) != entries.end();
}

bool SzDecompressor::extractTo(const std::string& outputDir)
{
    if (!szFile.is_open()) {
        throw std::runtime_error("7z file is not open");
    }

    if (entries.empty()) {
        return true;
    }

    if (!createDirectory(outputDir)) {
        throw std::runtime_error("Cannot create output directory: " + outputDir);
    }

    for (const auto& pair : entries) {
        const SevenZipEntry& entry = pair.second;

        if (!isSafeEntryName(entry.name)) {
            throw std::runtime_error("Unsafe entry name detected: " + entry.name);
        }

        std::string outputPath = joinPath(outputDir, entry.name);

        if (entry.isDirectory) {
            createDirectory(outputPath);
        } else {
            size_t pos = outputPath.find_last_of("/\\");
            if (pos != std::string::npos) {
                std::string dirPath = outputPath.substr(0, pos);
                createDirectory(dirPath);
            }

            extractFile(entry.name, outputPath);
        }
    }

    return true;
}

bool SzDecompressor::extractFile(const std::string& entryName, const std::string& outputPath)
{
    if (!szFile.is_open()) {
        throw std::runtime_error("7z file is not open");
    }

    if (!entryExists(entryName)) {
        throw std::runtime_error("Entry not found: " + entryName);
    }

    SevenZipEntry entry = getEntryInfo(entryName);

    if (!isSafeEntryName(entry.name)) {
        throw std::runtime_error("Unsafe entry name detected: " + entry.name);
    }

    if (entry.size > SevenZipLimits::MAX_UNCOMPRESSED_SIZE) {
        throw std::runtime_error("Uncompressed size exceeds maximum allowed for entry: " + entry.name);
    }

    std::vector<uint8_t> decompressedData;
    if (!decompressEntry(entry, decompressedData)) {
        throw std::runtime_error("Failed to decompress entry: " + entry.name);
    }

    if (decompressedData.size() != entry.size) {
        throw std::runtime_error("Decompressed size mismatch for entry: " + entry.name);
    }

    uint64_t calculatedCrc = Utils::LZMAUtils::calculateCRC64(decompressedData);
    if (entry.crc64 != 0 && calculatedCrc != entry.crc64) {
        throw std::runtime_error("CRC64 mismatch for entry: " + entry.name);
    }

    if (!writeFile(outputPath, decompressedData)) {
        return false;
    }

    return true;
}

bool SzDecompressor::decompressEntry(const SevenZipEntry& entry, std::vector<uint8_t>& output)
{
    if (entry.size == 0) {
        output.clear();
        return true;
    }

    szFile.seekg(static_cast<std::streamoff>(entry.dataOffset), std::ios::beg);
    if (!szFile.good()) {
        throw std::runtime_error("Cannot seek to entry data in 7z file");
    }

    std::vector<uint8_t> compressedData(entry.packedSize > 0 ? entry.packedSize : entry.size);
    szFile.read(reinterpret_cast<char*>(compressedData.data()), compressedData.size());
    if (!szFile.good()) {
        throw std::runtime_error("Cannot read compressed data from 7z file");
    }

    if (entry.compressionMethod == SevenZipMethods::METHOD_COPY) {
        output = compressedData;
        return true;
    }

    if (!entry.lzmaProps.empty()) {
        return Utils::LZMAUtils::decompressWithProps(compressedData, entry.lzmaProps, output, entry.size);
    }

    return Utils::LZMAUtils::decompress(compressedData, output, entry.size);
}

bool SzDecompressor::createDirectory(const std::string& path)
{
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return true;
    }

    size_t pos = 0;
    do {
        pos = path.find_first_of("/\\", pos + 1);
        std::string subPath;
        if (pos == std::string::npos) {
            subPath = path;
        } else {
            subPath = path.substr(0, pos);
        }

        if (!subPath.empty()) {
            attrs = GetFileAttributesA(subPath.c_str());
            if (attrs == INVALID_FILE_ATTRIBUTES) {
                if (!CreateDirectoryA(subPath.c_str(), nullptr)) {
                    if (GetLastError() != ERROR_ALREADY_EXISTS) {
                        return false;
                    }
                }
            }
        }
    } while (pos != std::string::npos);

    return true;
#else
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }

    size_t pos = 0;
    do {
        pos = path.find_first_of("/", pos + 1);
        std::string subPath;
        if (pos == std::string::npos) {
            subPath = path;
        } else {
            subPath = path.substr(0, pos);
        }

        if (!subPath.empty()) {
            if (stat(subPath.c_str(), &st) != 0) {
                if (mkdir(subPath.c_str(), 0755) != 0) {
                    return false;
                }
            }
        }
    } while (pos != std::string::npos);

    return true;
#endif
}

bool SzDecompressor::writeFile(const std::string& filePath, const std::vector<uint8_t>& data)
{
    std::ofstream outputFile(filePath, std::ios::binary | std::ios::trunc);
    if (!outputFile.is_open()) {
        throw std::runtime_error("Cannot open output file: " + filePath);
    }

    outputFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!outputFile.good()) {
        throw std::runtime_error("Cannot write to output file: " + filePath);
    }

    outputFile.close();
    return true;
}

bool SzDecompressor::close()
{
    if (szFile.is_open()) {
        szFile.close();
    }
    entries.clear();
    return true;
}

}
}
}
