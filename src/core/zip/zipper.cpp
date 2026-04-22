#include "compression/core/zip/zipper.h"

namespace Compression {
namespace Core {
namespace Zip {

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

    if (fileSize > static_cast<std::streamsize>(ZipLimits::MAX_UNCOMPRESSED_SIZE)) {
        throw std::runtime_error("File size exceeds maximum allowed: " + filePath);
    }

    std::vector<uint8_t> fileData(static_cast<size_t>(fileSize));
    if (!inputFile.read(reinterpret_cast<char*>(fileData.data()), fileSize)) {
        throw std::runtime_error("Cannot read input file: " + filePath);
    }
    inputFile.close();

    uint32_t crc32 = Utils::ZlibUtils::calculateCRC32(fileData);
    uint32_t uncompressedSize = static_cast<uint32_t>(fileData.size());

    std::vector<uint8_t> compressedData;
    bool compressed = Utils::ZlibUtils::compress(fileData, compressedData);

    uint32_t compressedSize;
    uint16_t compressionMethod;
    if (compressed && compressedData.size() < fileData.size()) {
        compressedSize = static_cast<uint32_t>(compressedData.size());
        compressionMethod = ZipMethods::DEFLATED;
    } else {
        compressedData = fileData;
        compressedSize = uncompressedSize;
        compressionMethod = ZipMethods::STORED;
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

    const uint16_t fileNameLength = static_cast<uint16_t>(entryName.size());

    zipFile.write(reinterpret_cast<const char*>(&ZipSignatures::LOCAL_FILE_HEADER), sizeof(ZipSignatures::LOCAL_FILE_HEADER));
    zipFile.write(reinterpret_cast<const char*>(&ZipVersions::VERSION_NEEDED), sizeof(ZipVersions::VERSION_NEEDED));
    zipFile.write(reinterpret_cast<const char*>(&ZipDefaults::GENERAL_PURPOSE_BIT_FLAG), sizeof(ZipDefaults::GENERAL_PURPOSE_BIT_FLAG));
    zipFile.write(reinterpret_cast<const char*>(&compressionMethod), sizeof(compressionMethod));
    zipFile.write(reinterpret_cast<const char*>(&ZipDefaults::LAST_MOD_TIME), sizeof(ZipDefaults::LAST_MOD_TIME));
    zipFile.write(reinterpret_cast<const char*>(&ZipDefaults::LAST_MOD_DATE), sizeof(ZipDefaults::LAST_MOD_DATE));
    zipFile.write(reinterpret_cast<const char*>(&crc32), sizeof(crc32));
    zipFile.write(reinterpret_cast<const char*>(&compressedSize), sizeof(compressedSize));
    zipFile.write(reinterpret_cast<const char*>(&uncompressedSize), sizeof(uncompressedSize));
    zipFile.write(reinterpret_cast<const char*>(&fileNameLength), sizeof(fileNameLength));
    zipFile.write(reinterpret_cast<const char*>(&ZipDefaults::EXTRA_FIELD_LENGTH), sizeof(ZipDefaults::EXTRA_FIELD_LENGTH));
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
    const uint16_t fileNameLength = static_cast<uint16_t>(entryName.size());

    std::vector<uint8_t> header;
    header.reserve(46 + fileNameLength);

    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipSignatures::CENTRAL_FILE_HEADER),
                  reinterpret_cast<const uint8_t*>(&ZipSignatures::CENTRAL_FILE_HEADER) + sizeof(ZipSignatures::CENTRAL_FILE_HEADER));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipVersions::VERSION_MADE_BY),
                  reinterpret_cast<const uint8_t*>(&ZipVersions::VERSION_MADE_BY) + sizeof(ZipVersions::VERSION_MADE_BY));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipVersions::VERSION_NEEDED),
                  reinterpret_cast<const uint8_t*>(&ZipVersions::VERSION_NEEDED) + sizeof(ZipVersions::VERSION_NEEDED));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipDefaults::GENERAL_PURPOSE_BIT_FLAG),
                  reinterpret_cast<const uint8_t*>(&ZipDefaults::GENERAL_PURPOSE_BIT_FLAG) + sizeof(ZipDefaults::GENERAL_PURPOSE_BIT_FLAG));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&compressionMethod),
                  reinterpret_cast<const uint8_t*>(&compressionMethod) + sizeof(compressionMethod));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipDefaults::LAST_MOD_TIME),
                  reinterpret_cast<const uint8_t*>(&ZipDefaults::LAST_MOD_TIME) + sizeof(ZipDefaults::LAST_MOD_TIME));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipDefaults::LAST_MOD_DATE),
                  reinterpret_cast<const uint8_t*>(&ZipDefaults::LAST_MOD_DATE) + sizeof(ZipDefaults::LAST_MOD_DATE));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&crc32),
                  reinterpret_cast<const uint8_t*>(&crc32) + sizeof(crc32));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&compressedSize),
                  reinterpret_cast<const uint8_t*>(&compressedSize) + sizeof(compressedSize));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&uncompressedSize),
                  reinterpret_cast<const uint8_t*>(&uncompressedSize) + sizeof(uncompressedSize));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&fileNameLength),
                  reinterpret_cast<const uint8_t*>(&fileNameLength) + sizeof(fileNameLength));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipDefaults::EXTRA_FIELD_LENGTH),
                  reinterpret_cast<const uint8_t*>(&ZipDefaults::EXTRA_FIELD_LENGTH) + sizeof(ZipDefaults::EXTRA_FIELD_LENGTH));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipDefaults::FILE_COMMENT_LENGTH),
                  reinterpret_cast<const uint8_t*>(&ZipDefaults::FILE_COMMENT_LENGTH) + sizeof(ZipDefaults::FILE_COMMENT_LENGTH));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipDefaults::DISK_NUMBER_START),
                  reinterpret_cast<const uint8_t*>(&ZipDefaults::DISK_NUMBER_START) + sizeof(ZipDefaults::DISK_NUMBER_START));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipDefaults::INTERNAL_FILE_ATTRIBUTES),
                  reinterpret_cast<const uint8_t*>(&ZipDefaults::INTERNAL_FILE_ATTRIBUTES) + sizeof(ZipDefaults::INTERNAL_FILE_ATTRIBUTES));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&ZipDefaults::EXTERNAL_FILE_ATTRIBUTES),
                  reinterpret_cast<const uint8_t*>(&ZipDefaults::EXTERNAL_FILE_ATTRIBUTES) + sizeof(ZipDefaults::EXTERNAL_FILE_ATTRIBUTES));
    header.insert(header.end(), reinterpret_cast<const uint8_t*>(&localHeaderOffset),
                  reinterpret_cast<const uint8_t*>(&localHeaderOffset) + sizeof(localHeaderOffset));
    header.insert(header.end(), entryName.begin(), entryName.end());

    centralDirectory.insert(centralDirectory.end(), header.begin(), header.end());
    return true;
}

bool Zipper::writeEndOfCentralDirectory()
{
    const uint16_t entriesOnThisDisk = entryCount;
    const uint16_t totalEntries = entryCount;

    zipFile.write(reinterpret_cast<const char*>(&ZipSignatures::END_OF_CENTRAL_DIR), sizeof(ZipSignatures::END_OF_CENTRAL_DIR));
    zipFile.write(reinterpret_cast<const char*>(&ZipDefaults::NUMBER_OF_THIS_DISK), sizeof(ZipDefaults::NUMBER_OF_THIS_DISK));
    zipFile.write(reinterpret_cast<const char*>(&ZipDefaults::DISK_WITH_CENTRAL_DIR), sizeof(ZipDefaults::DISK_WITH_CENTRAL_DIR));
    zipFile.write(reinterpret_cast<const char*>(&entriesOnThisDisk), sizeof(entriesOnThisDisk));
    zipFile.write(reinterpret_cast<const char*>(&totalEntries), sizeof(totalEntries));
    zipFile.write(reinterpret_cast<const char*>(&centralDirectorySize), sizeof(centralDirectorySize));
    zipFile.write(reinterpret_cast<const char*>(&centralDirectoryOffset), sizeof(centralDirectoryOffset));
    zipFile.write(reinterpret_cast<const char*>(&ZipDefaults::FILE_COMMENT_LENGTH), sizeof(ZipDefaults::FILE_COMMENT_LENGTH));

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

}
}
}
