#include "zipper.h"
#include "zlib_utils.h"

namespace Compression {

Zipper::Zipper()
    : centralDirectorySize(0)
    , centralDirectoryOffset(0)
    , entryCount(0)
{
}

Zipper::~Zipper()
{
    close();
}

bool Zipper::open(const std::string& zipPath)
{
    zipFile.open(zipPath, std::ios::binary | std::ios::trunc);
    if (!zipFile.is_open()) {
        throw std::runtime_error("Cannot open zip file for writing: " + zipPath);
    }
    centralDirectory.clear();
    centralDirectorySize = 0;
    centralDirectoryOffset = 0;
    entryCount = 0;
    return true;
}

bool Zipper::addFile(const std::string& filePath, const std::string& entryName)
{
    if (!zipFile.is_open()) {
        throw std::runtime_error("Zip file is not open");
    }

    std::ifstream inputFile(filePath, std::ios::binary | std::ios::ate);
    if (!inputFile.is_open()) {
        throw std::runtime_error("Cannot open input file: " + filePath);
    }

    std::streamsize fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    std::vector<uint8_t> fileData(static_cast<size_t>(fileSize));
    if (!inputFile.read(reinterpret_cast<char*>(fileData.data()), fileSize)) {
        throw std::runtime_error("Cannot read input file: " + filePath);
    }
    inputFile.close();

    uint32_t crc32 = ZlibUtils::calculateCRC32(fileData);
    uint32_t uncompressedSize = static_cast<uint32_t>(fileData.size());

    std::vector<uint8_t> compressedData;
    bool compressed = ZlibUtils::compress(fileData, compressedData);

    uint32_t compressedSize;
    uint16_t compressionMethod;
    if (compressed && compressedData.size() < fileData.size()) {
        compressedSize = static_cast<uint32_t>(compressedData.size());
        compressionMethod = 8;
    } else {
        compressedData = fileData;
        compressedSize = uncompressedSize;
        compressionMethod = 0;
    }

    std::string actualEntryName = entryName;
    if (actualEntryName.empty()) {
#ifdef _WIN32
        size_t pos = filePath.find_last_of("\\/");
#else
        size_t pos = filePath.find_last_of("/");
#endif
        actualEntryName = (pos == std::string::npos) ? filePath : filePath.substr(pos + 1);
    }

    uint32_t localHeaderOffset;
    if (!writeLocalFileHeader(actualEntryName, uncompressedSize, compressedSize,
                              crc32, compressionMethod, localHeaderOffset)) {
        return false;
    }

    zipFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedSize);
    if (!zipFile.good()) {
        throw std::runtime_error("Cannot write compressed data to zip file");
    }

    if (!writeCentralDirectoryHeader(actualEntryName, uncompressedSize, compressedSize,
                                     crc32, compressionMethod, localHeaderOffset)) {
        return false;
    }

    entryCount++;
    return true;
}

bool Zipper::addDirectory(const std::string& dirPath, const std::string& basePath)
{
    if (!zipFile.is_open()) {
        throw std::runtime_error("Zip file is not open");
    }

    if (!isDirectory(dirPath)) {
        throw std::runtime_error("Path is not a directory: " + dirPath);
    }

    std::vector<std::string> files;
    listFilesInDirectory(dirPath, files);

    for (const auto& filePath : files) {
        std::string relativePath = getRelativePath(dirPath, filePath);
        if (!basePath.empty()) {
            relativePath = basePath + "/" + relativePath;
        }
        addFile(filePath, relativePath);
    }

    return true;
}

bool Zipper::close()
{
    if (!zipFile.is_open()) {
        return true;
    }

    if (entryCount > 0) {
        centralDirectoryOffset = static_cast<uint32_t>(zipFile.tellp());
        centralDirectorySize = static_cast<uint32_t>(centralDirectory.size());

        zipFile.write(reinterpret_cast<const char*>(centralDirectory.data()), centralDirectory.size());
        if (!zipFile.good()) {
            throw std::runtime_error("Cannot write central directory to zip file");
        }

        if (!writeEndOfCentralDirectory()) {
            return false;
        }
    }

    zipFile.close();
    return true;
}

bool Zipper::writeLocalFileHeader(const std::string& entryName, uint32_t uncompressedSize,
                                   uint32_t compressedSize, uint32_t crc32,
                                   uint16_t compressionMethod, uint32_t& localHeaderOffset)
{
    localHeaderOffset = static_cast<uint32_t>(zipFile.tellp());

    const uint32_t localFileHeaderSignature = 0x04034b50;
    const uint16_t versionNeeded = 20;
    const uint16_t generalPurposeBitFlag = 0;
    const uint16_t lastModTime = 0;
    const uint16_t lastModDate = 0;
    const uint16_t extraFieldLength = 0;
    const uint16_t fileNameLength = static_cast<uint16_t>(entryName.size());

    zipFile.write(reinterpret_cast<const char*>(&localFileHeaderSignature), sizeof(localFileHeaderSignature));
    zipFile.write(reinterpret_cast<const char*>(&versionNeeded), sizeof(versionNeeded));
    zipFile.write(reinterpret_cast<const char*>(&generalPurposeBitFlag), sizeof(generalPurposeBitFlag));
    zipFile.write(reinterpret_cast<const char*>(&compressionMethod), sizeof(compressionMethod));
    zipFile.write(reinterpret_cast<const char*>(&lastModTime), sizeof(lastModTime));
    zipFile.write(reinterpret_cast<const char*>(&lastModDate), sizeof(lastModDate));
    zipFile.write(reinterpret_cast<const char*>(&crc32), sizeof(crc32));
    zipFile.write(reinterpret_cast<const char*>(&compressedSize), sizeof(compressedSize));
    zipFile.write(reinterpret_cast<const char*>(&uncompressedSize), sizeof(uncompressedSize));
    zipFile.write(reinterpret_cast<const char*>(&fileNameLength), sizeof(fileNameLength));
    zipFile.write(reinterpret_cast<const char*>(&extraFieldLength), sizeof(extraFieldLength));
    zipFile.write(entryName.data(), fileNameLength);

    if (!zipFile.good()) {
        throw std::runtime_error("Cannot write local file header to zip file");
    }

    return true;
}

bool Zipper::writeCentralDirectoryHeader(const std::string& entryName, uint32_t uncompressedSize,
                                          uint32_t compressedSize, uint32_t crc32,
                                          uint16_t compressionMethod, uint32_t localHeaderOffset)
{
    const uint32_t centralFileHeaderSignature = 0x02014b50;
    const uint16_t versionMadeBy = 20;
    const uint16_t versionNeeded = 20;
    const uint16_t generalPurposeBitFlag = 0;
    const uint16_t lastModTime = 0;
    const uint16_t lastModDate = 0;
    const uint16_t extraFieldLength = 0;
    const uint16_t fileCommentLength = 0;
    const uint16_t diskNumberStart = 0;
    const uint16_t internalFileAttributes = 0;
    const uint32_t externalFileAttributes = 0;
    const uint16_t fileNameLength = static_cast<uint16_t>(entryName.size());

    std::vector<uint8_t> header;
    header.reserve(46 + fileNameLength);

    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&centralFileHeaderSignature),
                  reinterpret_cast<const uint8_t*>(&centralFileHeaderSignature) + sizeof(centralFileHeaderSignature));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&versionMadeBy),
                  reinterpret_cast<const uint8_t*>(&versionMadeBy) + sizeof(versionMadeBy));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&versionNeeded),
                  reinterpret_cast<const uint8_t*>(&versionNeeded) + sizeof(versionNeeded));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&generalPurposeBitFlag),
                  reinterpret_cast<const uint8_t*>(&generalPurposeBitFlag) + sizeof(generalPurposeBitFlag));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&compressionMethod),
                  reinterpret_cast<const uint8_t*>(&compressionMethod) + sizeof(compressionMethod));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&lastModTime),
                  reinterpret_cast<const uint8_t*>(&lastModTime) + sizeof(lastModTime));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&lastModDate),
                  reinterpret_cast<const uint8_t*>(&lastModDate) + sizeof(lastModDate));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&crc32),
                  reinterpret_cast<const uint8_t*>(&crc32) + sizeof(crc32));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&compressedSize),
                  reinterpret_cast<const uint8_t*>(&compressedSize) + sizeof(compressedSize));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&uncompressedSize),
                  reinterpret_cast<const uint8_t*>(&uncompressedSize) + sizeof(uncompressedSize));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&fileNameLength),
                  reinterpret_cast<const uint8_t*>(&fileNameLength) + sizeof(fileNameLength));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&extraFieldLength),
                  reinterpret_cast<const uint8_t*>(&extraFieldLength) + sizeof(extraFieldLength));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&fileCommentLength),
                  reinterpret_cast<const uint8_t*>(&fileCommentLength) + sizeof(fileCommentLength));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&diskNumberStart),
                  reinterpret_cast<const uint8_t*>(&diskNumberStart) + sizeof(diskNumberStart));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&internalFileAttributes),
                  reinterpret_cast<const uint8_t*>(&internalFileAttributes) + sizeof(internalFileAttributes));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&externalFileAttributes),
                  reinterpret_cast<const uint8_t*>(&externalFileAttributes) + sizeof(externalFileAttributes));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&localHeaderOffset),
                  reinterpret_cast<const uint8_t*>(&localHeaderOffset) + sizeof(localHeaderOffset));
    header.insert(header.end(), entryName.begin(), entryName.end());

    centralDirectory.insert(centralDirectory.end(), header.begin(), header.end());
    return true;
}

bool Zipper::writeEndOfCentralDirectory()
{
    const uint32_t endOfCentralDirSignature = 0x06054b50;
    const uint16_t numberOfThisDisk = 0;
    const uint16_t diskWithCentralDir = 0;
    const uint16_t entriesOnThisDisk = entryCount;
    const uint16_t totalEntries = entryCount;
    const uint16_t commentLength = 0;

    zipFile.write(reinterpret_cast<const char*>(&endOfCentralDirSignature), sizeof(endOfCentralDirSignature));
    zipFile.write(reinterpret_cast<const char*>(&numberOfThisDisk), sizeof(numberOfThisDisk));
    zipFile.write(reinterpret_cast<const char*>(&diskWithCentralDir), sizeof(diskWithCentralDir));
    zipFile.write(reinterpret_cast<const char*>(&entriesOnThisDisk), sizeof(entriesOnThisDisk));
    zipFile.write(reinterpret_cast<const char*>(&totalEntries), sizeof(totalEntries));
    zipFile.write(reinterpret_cast<const char*>(&centralDirectorySize), sizeof(centralDirectorySize));
    zipFile.write(reinterpret_cast<const char*>(&centralDirectoryOffset), sizeof(centralDirectoryOffset));
    zipFile.write(reinterpret_cast<const char*>(&commentLength), sizeof(commentLength));

    if (!zipFile.good()) {
        throw std::runtime_error("Cannot write end of central directory to zip file");
    }

    return true;
}

bool Zipper::isDirectory(const std::string& path)
{
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
#endif
}

void Zipper::listFilesInDirectory(const std::string& dirPath, std::vector<std::string>& files)
{
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = dirPath + "\\*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Cannot open directory: " + dirPath);
    }

    do {
        std::string fileName = findData.cFileName;
        if (fileName == "." || fileName == "..") {
            continue;
        }

        std::string fullPath = dirPath + "\\" + fileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            listFilesInDirectory(fullPath, files);
        } else {
            files.push_back(fullPath);
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
#else
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) {
        throw std::runtime_error("Cannot open directory: " + dirPath);
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string fileName = entry->d_name;
        if (fileName == "." || fileName == "..") {
            continue;
        }

        std::string fullPath = dirPath + "/" + fileName;

        if (isDirectory(fullPath)) {
            listFilesInDirectory(fullPath, files);
        } else {
            files.push_back(fullPath);
        }
    }

    closedir(dir);
#endif
}

std::string Zipper::getRelativePath(const std::string& basePath, const std::string& fullPath)
{
    if (fullPath.find(basePath) != 0) {
        return fullPath;
    }

    size_t baseLen = basePath.length();
    if (baseLen == 0) {
        return fullPath;
    }

    std::string relative = fullPath.substr(baseLen);

#ifdef _WIN32
    if (relative[0] == '\\' || relative[0] == '/') {
        relative = relative.substr(1);
    }
    for (char& c : relative) {
        if (c == '\\') {
            c = '/';
        }
    }
#else
    if (relative[0] == '/') {
        relative = relative.substr(1);
    }
#endif

    return relative;
}

} // namespace Compression
