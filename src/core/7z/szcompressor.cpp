#include "compression/core/7z/szcompressor.h"
#include <cstring>
#include <ctime>

namespace Compression {
namespace Core {
namespace SevenZip {

SzCompressor::SzCompressor()
    : compressionLevel(6)
    , headerOffset(0)
{
}

SzCompressor::~SzCompressor()
{
    close();
}

void SzCompressor::setCompressionLevel(int level)
{
    if (level >= 0 && level <= 9) {
        compressionLevel = level;
    }
}

int SzCompressor::getCompressionLevel() const
{
    return compressionLevel;
}

bool SzCompressor::open(const std::string& szPath)
{
    szFile.open(szPath, std::ios::binary | std::ios::trunc);
    if (!szFile.is_open()) {
        throw std::runtime_error("Cannot open 7z file for writing: " + szPath);
    }

    fileInfos.clear();
    compressedData.clear();

    if (!writeSignatureHeader()) {
        return false;
    }

    return true;
}

bool SzCompressor::writeSignatureHeader()
{
    uint8_t signature[6] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};
    szFile.write(reinterpret_cast<const char*>(signature), 6);

    uint8_t majorVersion = SevenZipVersions::MAJOR_VERSION;
    uint8_t minorVersion = SevenZipVersions::MINOR_VERSION;
    szFile.write(reinterpret_cast<const char*>(&majorVersion), 1);
    szFile.write(reinterpret_cast<const char*>(&minorVersion), 1);

    uint32_t startHeaderCRC = 0;
    uint64_t nextHeaderOffset = 0;
    uint64_t nextHeaderSize = 0;
    uint32_t nextHeaderCRC = 0;

    szFile.write(reinterpret_cast<const char*>(&startHeaderCRC), 4);
    szFile.write(reinterpret_cast<const char*>(&nextHeaderOffset), 8);
    szFile.write(reinterpret_cast<const char*>(&nextHeaderSize), 8);
    szFile.write(reinterpret_cast<const char*>(&nextHeaderCRC), 4);

    if (!szFile.good()) {
        throw std::runtime_error("Cannot write signature header to 7z file");
    }

    return true;
}

bool SzCompressor::addFile(const std::string& filePath, const std::string& entryName)
{
    if (!szFile.is_open()) {
        throw std::runtime_error("7z file is not open");
    }

    if (isDirectory(filePath)) {
        throw std::runtime_error("Path is a directory, use addDirectory instead: " + filePath);
    }

    return compressAndAddFile(filePath, entryName);
}

bool SzCompressor::addDirectory(const std::string& dirPath, const std::string& basePath)
{
    if (!szFile.is_open()) {
        throw std::runtime_error("7z file is not open");
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
        compressAndAddFile(filePath, relativePath);
    }

    return true;
}

bool SzCompressor::compressAndAddFile(const std::string& filePath, const std::string& entryName)
{
    std::vector<uint8_t> fileData = readFileContent(filePath);

    if (fileData.size() > SevenZipLimits::MAX_SINGLE_FILE_SIZE) {
        throw std::runtime_error("File size exceeds maximum allowed: " + filePath);
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

    Utils::LZMAUtils::LZMAConfig config;
    config.dictSize = 1 << (20 + compressionLevel);
    if (config.dictSize > (1 << 26)) {
        config.dictSize = 1 << 26;
    }

    std::vector<uint8_t> compressed;
    std::vector<uint8_t> props;

    if (fileData.size() > 0) {
        if (!Utils::LZMAUtils::compressWithProps(fileData, compressed, props, config)) {
            compressed = fileData;
            props.clear();
        }
    }

    SevenZipFileInfo info;
    info.name = actualEntryName;
    info.size = fileData.size();
    info.packedSize = compressed.size();
    info.crc64 = Utils::LZMAUtils::calculateCRC64(fileData);
    info.isDirectory = false;
    info.lzmaProps = props;
    info.dataOffset = compressedData.size();

#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA fileAttrs;
    if (GetFileAttributesExA(filePath.c_str(), GetFileExInfoStandard, &fileAttrs)) {
        info.attributes = fileAttrs.dwFileAttributes;
        ULARGE_INTEGER ft;
        ft.LowPart = fileAttrs.ftLastWriteTime.dwLowDateTime;
        ft.HighPart = fileAttrs.ftLastWriteTime.dwHighDateTime;
        info.modificationTime = ft.QuadPart;
    } else {
        info.attributes = 0;
        info.modificationTime = 0;
    }
#else
    struct stat st;
    if (stat(filePath.c_str(), &st) == 0) {
        info.attributes = st.st_mode;
        info.modificationTime = static_cast<uint64_t>(st.st_mtime) * 10000000ULL + 116444736000000000ULL;
    } else {
        info.attributes = 0;
        info.modificationTime = 0;
    }
#endif

    compressedData.insert(compressedData.end(), compressed.begin(), compressed.end());
    fileInfos.push_back(info);

    return true;
}

std::vector<uint8_t> SzCompressor::readFileContent(const std::string& filePath)
{
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

    return fileData;
}

bool SzCompressor::close()
{
    if (!szFile.is_open()) {
        return true;
    }

    if (fileInfos.empty() && compressedData.empty()) {
        szFile.close();
        return true;
    }

    if (!compressedData.empty()) {
        szFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());
        if (!szFile.good()) {
            throw std::runtime_error("Cannot write compressed data to 7z file");
        }
    }

    uint64_t headerOffsetPos = static_cast<uint64_t>(szFile.tellp());

    if (!writeHeader()) {
        return false;
    }

    uint64_t headerEndPos = static_cast<uint64_t>(szFile.tellp());
    uint64_t headerSize = headerEndPos - headerOffsetPos;

    szFile.seekp(12, std::ios::beg);
    szFile.write(reinterpret_cast<const char*>(&headerOffsetPos), 8);
    szFile.write(reinterpret_cast<const char*>(&headerSize), 8);

    szFile.close();
    return true;
}

bool SzCompressor::writeHeader()
{
    std::vector<uint8_t> headerData;

    headerData.push_back(SevenZipIDs::ID_MAIN_STREAMS_INFO);
    if (!writeStreamsInfoForHeader(headerData)) {
        return false;
    }

    headerData.push_back(SevenZipIDs::ID_FILES_INFO);
    if (!writeFilesInfoForHeader(headerData)) {
        return false;
    }

    headerData.push_back(SevenZipIDs::ID_END);

    uint32_t headerCRC = 0;

    szFile.write(reinterpret_cast<const char*>(headerData.data()), headerData.size());
    szFile.write(reinterpret_cast<const char*>(&headerCRC), 4);

    if (!szFile.good()) {
        throw std::runtime_error("Cannot write header to 7z file");
    }

    return true;
}

bool SzCompressor::writeStreamsInfoForHeader(std::vector<uint8_t>& output)
{
    output.push_back(SevenZipIDs::ID_PACK_INFO);
    Utils::LZMAUtils::writeVarInt(output, 1);
    Utils::LZMAUtils::writeVarInt(output, compressedData.size());

    uint64_t totalUnpackSize = 0;
    for (const auto& info : fileInfos) {
        totalUnpackSize += info.size;
    }

    output.push_back(SevenZipIDs::ID_UNPACK_INFO);
    output.push_back(SevenZipIDs::ID_FOLDER);
    Utils::LZMAUtils::writeVarInt(output, 1);
    Utils::LZMAUtils::writeVarInt(output, 1);

    output.push_back(SevenZipIDs::ID_CODERS_UNPACK_SIZE);
    Utils::LZMAUtils::writeVarInt(output, totalUnpackSize);

    output.push_back(SevenZipIDs::ID_SUBSTREAMS_INFO);
    Utils::LZMAUtils::writeVarInt(output, fileInfos.size());

    for (const auto& info : fileInfos) {
        Utils::LZMAUtils::writeVarInt(output, info.size);
    }

    return true;
}

bool SzCompressor::writeFilesInfoForHeader(std::vector<uint8_t>& output)
{
    Utils::LZMAUtils::writeVarInt(output, fileInfos.size());

    std::vector<std::string> names;
    for (const auto& info : fileInfos) {
        names.push_back(info.name);
    }

    output.push_back(SevenZipIDs::ID_NAME);
    for (const auto& name : names) {
        for (char c : name) {
            output.push_back(static_cast<uint8_t>(c));
        }
        output.push_back(0);
    }

    output.push_back(SevenZipIDs::ID_SIZE);
    for (const auto& info : fileInfos) {
        Utils::LZMAUtils::writeVarInt(output, info.size);
    }

    output.push_back(SevenZipIDs::ID_CRC);
    for (const auto& info : fileInfos) {
        output.insert(output.end(), 
                       reinterpret_cast<const uint8_t*>(&info.crc64),
                       reinterpret_cast<const uint8_t*>(&info.crc64) + 8);
    }

    return true;
}

bool SzCompressor::isDirectory(const std::string& path)
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

void SzCompressor::listFilesInDirectory(const std::string& dirPath, std::vector<std::string>& files)
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

std::string SzCompressor::getRelativePath(const std::string& basePath, const std::string& fullPath)
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
