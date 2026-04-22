#include "unzipper.h"
#include "zlib_utils.h"

namespace Compression {

bool Unzipper::isSafeEntryName(const std::string& entryName)
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

    if (entryName.find("\\") != std::string::npos) {
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

std::string Unzipper::joinPath(const std::string& base, const std::string& name)
{
#ifdef _WIN32
    return base + "\\" + name;
#else
    return base + "/" + name;
#endif
}

Unzipper::Unzipper()
{
}

Unzipper::~Unzipper()
{
    close();
}

bool Unzipper::open(const std::string& zipPath)
{
    zipFile.open(zipPath, std::ios::binary);
    if (!zipFile.is_open()) {
        throw std::runtime_error("Cannot open zip file for reading: " + zipPath);
    }

    entries.clear();

    if (!readCentralDirectory()) {
        close();
        return false;
    }

    return true;
}

bool Unzipper::extractTo(const std::string& outputDir)
{
    if (!zipFile.is_open()) {
        throw std::runtime_error("Zip file is not open");
    }

    if (entries.empty()) {
        return true;
    }

    if (!createDirectory(outputDir)) {
        throw std::runtime_error("Cannot create output directory: " + outputDir);
    }

    for (const auto& pair : entries) {
        const ZipEntryInfo& entry = pair.second;

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

bool Unzipper::extractFile(const std::string& entryName, const std::string& outputPath)
{
    if (!zipFile.is_open()) {
        throw std::runtime_error("Zip file is not open");
    }

    if (!entryExists(entryName)) {
        throw std::runtime_error("Entry not found: " + entryName);
    }

    ZipEntryInfo entry = getEntryInfo(entryName);

    uint32_t dataOffset;
    uint32_t compressedSize;
    uint16_t compressionMethod;

    if (!readLocalFileHeader(entry.localHeaderOffset, dataOffset, compressedSize, compressionMethod)) {
        return false;
    }

    zipFile.seekg(dataOffset, std::ios::beg);
    if (!zipFile.good()) {
        throw std::runtime_error("Cannot seek to entry data in zip file");
    }

    std::vector<uint8_t> compressedData(compressedSize);
    zipFile.read(reinterpret_cast<char*>(compressedData.data()), compressedSize);
    if (!zipFile.good()) {
        throw std::runtime_error("Cannot read compressed data from zip file");
    }

    std::vector<uint8_t> decompressedData;

    if (compressionMethod == 0) {
        decompressedData = compressedData;
    } else if (compressionMethod == 8) {
        if (!ZlibUtils::decompress(compressedData, decompressedData, entry.uncompressedSize)) {
            throw std::runtime_error("Cannot decompress entry: " + entryName);
        }
    } else {
        throw std::runtime_error("Unsupported compression method: " + std::to_string(compressionMethod));
    }

    if (decompressedData.size() != entry.uncompressedSize) {
        throw std::runtime_error("Decompressed size mismatch for entry: " + entryName);
    }

    uint32_t calculatedCrc = ZlibUtils::calculateCRC32(decompressedData);
    if (calculatedCrc != entry.crc32) {
        throw std::runtime_error("CRC32 mismatch for entry: " + entryName);
    }

    if (!writeFile(outputPath, decompressedData)) {
        return false;
    }

    return true;
}

std::vector<std::string> Unzipper::listEntries()
{
    std::vector<std::string> names;
    for (const auto& pair : entries) {
        names.push_back(pair.first);
    }
    return names;
}

ZipEntryInfo Unzipper::getEntryInfo(const std::string& entryName)
{
    if (!entryExists(entryName)) {
        throw std::runtime_error("Entry not found: " + entryName);
    }
    return entries[entryName];
}

bool Unzipper::close()
{
    if (zipFile.is_open()) {
        zipFile.close();
    }
    entries.clear();
    return true;
}

bool Unzipper::readCentralDirectory()
{
    uint32_t centralDirectoryOffset;
    uint16_t entryCount;

    if (!readEndOfCentralDirectory(centralDirectoryOffset, entryCount)) {
        return false;
    }

    zipFile.seekg(centralDirectoryOffset, std::ios::beg);
    if (!zipFile.good()) {
        throw std::runtime_error("Cannot seek to central directory");
    }

    for (uint16_t i = 0; i < entryCount; ++i) {
        uint32_t signature;
        zipFile.read(reinterpret_cast<char*>(&signature), sizeof(signature));

        if (signature != 0x02014b50) {
            throw std::runtime_error("Invalid central directory entry signature");
        }

        uint16_t versionMadeBy;
        uint16_t versionNeeded;
        uint16_t generalPurposeBitFlag;
        uint16_t compressionMethod;
        uint16_t lastModTime;
        uint16_t lastModDate;
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t uncompressedSize;
        uint16_t fileNameLength;
        uint16_t extraFieldLength;
        uint16_t fileCommentLength;
        uint16_t diskNumberStart;
        uint16_t internalFileAttributes;
        uint32_t externalFileAttributes;
        uint32_t localHeaderOffset;

        zipFile.read(reinterpret_cast<char*>(&versionMadeBy), sizeof(versionMadeBy));
        zipFile.read(reinterpret_cast<char*>(&versionNeeded), sizeof(versionNeeded));
        zipFile.read(reinterpret_cast<char*>(&generalPurposeBitFlag), sizeof(generalPurposeBitFlag));
        zipFile.read(reinterpret_cast<char*>(&compressionMethod), sizeof(compressionMethod));
        zipFile.read(reinterpret_cast<char*>(&lastModTime), sizeof(lastModTime));
        zipFile.read(reinterpret_cast<char*>(&lastModDate), sizeof(lastModDate));
        zipFile.read(reinterpret_cast<char*>(&crc32), sizeof(crc32));
        zipFile.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
        zipFile.read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uncompressedSize));
        zipFile.read(reinterpret_cast<char*>(&fileNameLength), sizeof(fileNameLength));
        zipFile.read(reinterpret_cast<char*>(&extraFieldLength), sizeof(extraFieldLength));
        zipFile.read(reinterpret_cast<char*>(&fileCommentLength), sizeof(fileCommentLength));
        zipFile.read(reinterpret_cast<char*>(&diskNumberStart), sizeof(diskNumberStart));
        zipFile.read(reinterpret_cast<char*>(&internalFileAttributes), sizeof(internalFileAttributes));
        zipFile.read(reinterpret_cast<char*>(&externalFileAttributes), sizeof(externalFileAttributes));
        zipFile.read(reinterpret_cast<char*>(&localHeaderOffset), sizeof(localHeaderOffset));

        std::string entryName(fileNameLength, 0);
        zipFile.read(&entryName[0], fileNameLength);

        zipFile.seekg(extraFieldLength + fileCommentLength, std::ios::cur);

        bool isDirectory = (!entryName.empty() && entryName.back() == '/');

        ZipEntryInfo info;
        info.name = entryName;
        info.compressedSize = compressedSize;
        info.uncompressedSize = uncompressedSize;
        info.crc32 = crc32;
        info.localHeaderOffset = localHeaderOffset;
        info.compressionMethod = compressionMethod;
        info.isDirectory = isDirectory;

        entries[entryName] = info;
    }

    return true;
}

bool Unzipper::readEndOfCentralDirectory(uint32_t& centralDirectoryOffset, uint16_t& entryCount)
{
    const uint32_t endOfCentralDirSignature = 0x06054b50;
    const size_t minSize = 22;

    zipFile.seekg(0, std::ios::end);
    std::streampos fileSize = zipFile.tellg();

    if (fileSize < static_cast<std::streampos>(minSize)) {
        throw std::runtime_error("Zip file is too small");
    }

    std::streampos searchStart = fileSize - static_cast<std::streampos>(65536 + minSize);
    if (searchStart < static_cast<std::streampos>(0)) {
        searchStart = static_cast<std::streampos>(0);
    }
    size_t searchSize = static_cast<size_t>(fileSize - searchStart);

    zipFile.seekg(searchStart, std::ios::beg);
    std::vector<uint8_t> buffer(searchSize);
    zipFile.read(reinterpret_cast<char*>(buffer.data()), searchSize);

    int64_t offset = -1;
    for (size_t i = 0; i <= buffer.size() - minSize; ++i) {
        uint32_t signature = *reinterpret_cast<const uint32_t*>(&buffer[i]);
        if (signature == endOfCentralDirSignature) {
            offset = static_cast<int64_t>(searchStart) + static_cast<int64_t>(i);
            break;
        }
    }

    if (offset == -1) {
        throw std::runtime_error("Cannot find end of central directory record");
    }

    zipFile.seekg(static_cast<std::streamoff>(offset + 10), std::ios::beg);

    uint16_t entriesOnThisDisk;
    zipFile.read(reinterpret_cast<char*>(&entriesOnThisDisk), sizeof(entriesOnThisDisk));

    zipFile.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));

    uint32_t centralDirectorySize;
    zipFile.read(reinterpret_cast<char*>(&centralDirectorySize), sizeof(centralDirectorySize));

    zipFile.read(reinterpret_cast<char*>(&centralDirectoryOffset), sizeof(centralDirectoryOffset));

    return true;
}

bool Unzipper::readLocalFileHeader(uint32_t localHeaderOffset, uint32_t& dataOffset,
                                     uint32_t& compressedSize, uint16_t& compressionMethod)
{
    zipFile.seekg(localHeaderOffset, std::ios::beg);
    if (!zipFile.good()) {
        throw std::runtime_error("Cannot seek to local file header");
    }

    uint32_t signature;
    zipFile.read(reinterpret_cast<char*>(&signature), sizeof(signature));

    if (signature != 0x04034b50) {
        throw std::runtime_error("Invalid local file header signature");
    }

    uint16_t versionNeeded;
    uint16_t generalPurposeBitFlag;
    uint32_t crc32;
    uint32_t uncompressedSize;
    uint16_t fileNameLength;
    uint16_t extraFieldLength;

    zipFile.read(reinterpret_cast<char*>(&versionNeeded), sizeof(versionNeeded));
    zipFile.read(reinterpret_cast<char*>(&generalPurposeBitFlag), sizeof(generalPurposeBitFlag));
    zipFile.read(reinterpret_cast<char*>(&compressionMethod), sizeof(compressionMethod));
    zipFile.seekg(4, std::ios::cur);
    zipFile.read(reinterpret_cast<char*>(&crc32), sizeof(crc32));
    zipFile.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
    zipFile.read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uncompressedSize));
    zipFile.read(reinterpret_cast<char*>(&fileNameLength), sizeof(fileNameLength));
    zipFile.read(reinterpret_cast<char*>(&extraFieldLength), sizeof(extraFieldLength));

    zipFile.seekg(fileNameLength + extraFieldLength, std::ios::cur);

    dataOffset = static_cast<uint32_t>(zipFile.tellg());
    return true;
}

bool Unzipper::createDirectory(const std::string& path)
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

bool Unzipper::writeFile(const std::string& filePath, const std::vector<uint8_t>& data)
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

bool Unzipper::entryExists(const std::string& entryName)
{
    return entries.find(entryName) != entries.end();
}

} // namespace Compression
