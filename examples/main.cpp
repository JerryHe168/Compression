#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif
#include "zipper.h"
#include "unzipper.h"

bool createDirectory(const std::string& path) {
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

void createTestFiles(const std::string& testDir) {
    std::cout << "Creating test files in: " << testDir << std::endl;

    createDirectory(testDir);
    createDirectory(testDir + "/subdir");

    std::ofstream file1(testDir + "/file1.txt");
    file1 << "Hello, this is a test file for compression.\n";
    file1 << "This file contains some sample text that will be compressed.\n";
    file1 << "Compression is the process of encoding information using fewer bits\n";
    file1 << "than the original representation.\n";
    file1.close();
    std::cout << "  Created: file1.txt" << std::endl;

    std::ofstream file2(testDir + "/subdir/file2.txt");
    file2 << "This is another test file located in a subdirectory.\n";
    file2 << "Testing directory compression and decompression.\n";
    file2.close();
    std::cout << "  Created: subdir/file2.txt" << std::endl;

    std::ofstream file3(testDir + "/binary.bin", std::ios::binary);
    std::vector<uint8_t> binaryData;
    for (int i = 0; i < 1024; ++i) {
        binaryData.push_back(static_cast<uint8_t>(i % 256));
    }
    file3.write(reinterpret_cast<char*>(binaryData.data()), binaryData.size());
    file3.close();
    std::cout << "  Created: binary.bin (1KB)" << std::endl;
}

void testCompression(const std::string& testDir, const std::string& zipPath) {
    std::cout << "\n=== Testing Compression ===" << std::endl;
    std::cout << "Compressing directory: " << testDir << std::endl;
    std::cout << "To zip file: " << zipPath << std::endl;

    Compression::Zipper zipper;

    try {
        zipper.open(zipPath);

        std::cout << "Adding files..." << std::endl;
        zipper.addDirectory(testDir);

        zipper.close();

        std::cout << "Compression completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Compression failed: " << e.what() << std::endl;
        throw;
    }
}

void testListing(const std::string& zipPath) {
    std::cout << "\n=== Testing Entry Listing ===" << std::endl;
    std::cout << "Reading zip file: " << zipPath << std::endl;

    Compression::Unzipper unzipper;

    try {
        unzipper.open(zipPath);

        std::vector<std::string> entries = unzipper.listEntries();
        std::cout << "\nEntries in zip file (" << entries.size() << " files):" << std::endl;
        for (const auto& entry : entries) {
            Compression::ZipEntryInfo info = unzipper.getEntryInfo(entry);
            std::cout << "  - " << entry << std::endl;
            std::cout << "    Compressed size: " << info.compressedSize << " bytes" << std::endl;
            std::cout << "    Uncompressed size: " << info.uncompressedSize << " bytes" << std::endl;
            if (info.compressedSize < info.uncompressedSize && info.uncompressedSize > 0) {
                double ratio = (1.0 - static_cast<double>(info.compressedSize) / info.uncompressedSize) * 100.0;
                std::cout << "    Compression ratio: " << ratio << "%" << std::endl;
            }
        }

        unzipper.close();
        std::cout << "\nListing completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Listing failed: " << e.what() << std::endl;
        throw;
    }
}

void testDecompression(const std::string& zipPath, const std::string& outputDir) {
    std::cout << "\n=== Testing Decompression ===" << std::endl;
    std::cout << "Extracting zip file: " << zipPath << std::endl;
    std::cout << "To directory: " << outputDir << std::endl;

    Compression::Unzipper unzipper;

    try {
        unzipper.open(zipPath);
        unzipper.extractTo(outputDir);
        unzipper.close();

        std::cout << "Decompression completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Decompression failed: " << e.what() << std::endl;
        throw;
    }
}

void testSingleFileOperations(const std::string& testDir, const std::string& zipPath) {
    std::cout << "\n=== Testing Single File Operations ===" << std::endl;

    Compression::Zipper zipper;
    try {
        zipper.open(zipPath);

        std::cout << "Adding single file: file1.txt" << std::endl;
        zipper.addFile(testDir + "/file1.txt", "single_file.txt");

        std::cout << "Adding another file with custom name: renamed_bin.bin" << std::endl;
        zipper.addFile(testDir + "/binary.bin", "data/renamed_bin.bin");

        zipper.close();
        std::cout << "Single file compression completed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Single file compression failed: " << e.what() << std::endl;
        throw;
    }

    std::cout << "\nListing entries in single file zip:" << std::endl;
    Compression::Unzipper unzipper;
    try {
        unzipper.open(zipPath);
        std::vector<std::string> entries = unzipper.listEntries();
        for (const auto& entry : entries) {
            std::cout << "  - " << entry << std::endl;
        }
        unzipper.close();
    } catch (const std::exception& e) {
        std::cerr << "Listing failed: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Compression Library Test Suite     " << std::endl;
    std::cout << "========================================" << std::endl;

    std::string testDir = "./test_files";
    std::string zipPath = "./test_output.zip";
    std::string singleZipPath = "./single_file_test.zip";
    std::string outputDir = "./extracted_files";

    try {
        createTestFiles(testDir);
        testCompression(testDir, zipPath);
        testListing(zipPath);
        testDecompression(zipPath, outputDir);
        testSingleFileOperations(testDir, singleZipPath);

        std::cout << "\n========================================" << std::endl;
        std::cout << "      All tests completed successfully!   " << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\nGenerated files:" << std::endl;
        std::cout << "  - " << zipPath << std::endl;
        std::cout << "  - " << singleZipPath << std::endl;
        std::cout << "  - " << outputDir << "/" << std::endl;
        std::cout << "  - " << testDir << "/" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
