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
#include "compression/core/zip/zipper.h"
#include "compression/core/zip/unzipper.h"
#include "compression/core/7z/szcompressor.h"
#include "compression/core/7z/szdecompressor.h"
#include "compression/utils/lzma_utils.h"

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

void testZipCompression(const std::string& testDir, const std::string& zipPath) {
    std::cout << "\n=== Testing ZIP Compression ===" << std::endl;
    std::cout << "Compressing directory: " << testDir << std::endl;
    std::cout << "To zip file: " << zipPath << std::endl;

    Compression::Core::Zip::Zipper zipper;

    try {
        zipper.open(zipPath);

        std::cout << "Adding files..." << std::endl;
        zipper.addDirectory(testDir);

        zipper.close();

        std::cout << "ZIP compression completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ZIP compression failed: " << e.what() << std::endl;
        throw;
    }
}

void testZipListing(const std::string& zipPath) {
    std::cout << "\n=== Testing ZIP Entry Listing ===" << std::endl;
    std::cout << "Reading zip file: " << zipPath << std::endl;

    Compression::Core::Zip::Unzipper unzipper;

    try {
        unzipper.open(zipPath);

        std::vector<std::string> entries = unzipper.listEntries();
        std::cout << "\nEntries in zip file (" << entries.size() << " files):" << std::endl;
        for (const auto& entry : entries) {
            Compression::Core::Zip::ZipEntryInfo info = unzipper.getEntryInfo(entry);
            std::cout << "  - " << entry << std::endl;
            std::cout << "    Compressed size: " << info.compressedSize << " bytes" << std::endl;
            std::cout << "    Uncompressed size: " << info.uncompressedSize << " bytes" << std::endl;
            if (info.compressedSize < info.uncompressedSize && info.uncompressedSize > 0) {
                double ratio = (1.0 - static_cast<double>(info.compressedSize) / info.uncompressedSize) * 100.0;
                std::cout << "    Compression ratio: " << ratio << "%" << std::endl;
            }
        }

        unzipper.close();
        std::cout << "\nZIP listing completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ZIP listing failed: " << e.what() << std::endl;
        throw;
    }
}

void testZipDecompression(const std::string& zipPath, const std::string& outputDir) {
    std::cout << "\n=== Testing ZIP Decompression ===" << std::endl;
    std::cout << "Extracting zip file: " << zipPath << std::endl;
    std::cout << "To directory: " << outputDir << std::endl;

    Compression::Core::Zip::Unzipper unzipper;

    try {
        unzipper.open(zipPath);
        unzipper.extractTo(outputDir);
        unzipper.close();

        std::cout << "ZIP decompression completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ZIP decompression failed: " << e.what() << std::endl;
        throw;
    }
}

void test7zCompression(const std::string& testDir, const std::string& szPath) {
    std::cout << "\n=== Testing 7z Compression ===" << std::endl;
    std::cout << "Compressing directory: " << testDir << std::endl;
    std::cout << "To 7z file: " << szPath << std::endl;

    Compression::Core::SevenZip::SzCompressor compressor;

    try {
        compressor.open(szPath);

        std::cout << "LZMA available: " << (Compression::Utils::LZMAUtils::isLzmaAvailable() ? "Yes" : "No") << std::endl;
        std::cout << "Adding files..." << std::endl;
        compressor.addDirectory(testDir);

        compressor.close();

        std::cout << "7z compression completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "7z compression failed: " << e.what() << std::endl;
        throw;
    }
}

void test7zListing(const std::string& szPath) {
    std::cout << "\n=== Testing 7z Entry Listing ===" << std::endl;
    std::cout << "Reading 7z file: " << szPath << std::endl;

    Compression::Core::SevenZip::SzDecompressor decompressor;

    try {
        decompressor.open(szPath);

        std::vector<std::string> entries = decompressor.listEntries();
        std::cout << "\nEntries in 7z file (" << entries.size() << " files):" << std::endl;
        for (const auto& entry : entries) {
            Compression::Core::SevenZip::SevenZipEntry info = decompressor.getEntryInfo(entry);
            std::cout << "  - " << entry << std::endl;
            std::cout << "    Packed size: " << info.packedSize << " bytes" << std::endl;
            std::cout << "    Uncompressed size: " << info.size << " bytes" << std::endl;
            if (info.packedSize < info.size && info.size > 0) {
                double ratio = (1.0 - static_cast<double>(info.packedSize) / info.size) * 100.0;
                std::cout << "    Compression ratio: " << ratio << "%" << std::endl;
            }
        }

        decompressor.close();
        std::cout << "\n7z listing completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "7z listing failed: " << e.what() << std::endl;
        throw;
    }
}

void test7zDecompression(const std::string& szPath, const std::string& outputDir) {
    std::cout << "\n=== Testing 7z Decompression ===" << std::endl;
    std::cout << "Extracting 7z file: " << szPath << std::endl;
    std::cout << "To directory: " << outputDir << std::endl;

    Compression::Core::SevenZip::SzDecompressor decompressor;

    try {
        decompressor.open(szPath);
        decompressor.extractTo(outputDir);
        decompressor.close();

        std::cout << "7z decompression completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "7z decompression failed: " << e.what() << std::endl;
        throw;
    }
}

void test7zSingleFileOperations(const std::string& testDir, const std::string& szPath) {
    std::cout << "\n=== Testing 7z Single File Operations ===" << std::endl;

    Compression::Core::SevenZip::SzCompressor compressor;
    try {
        compressor.open(szPath);

        std::cout << "Adding single file: file1.txt" << std::endl;
        compressor.addFile(testDir + "/file1.txt", "single_file.txt");

        std::cout << "Adding another file with custom name: renamed_bin.bin" << std::endl;
        compressor.addFile(testDir + "/binary.bin", "data/renamed_bin.bin");

        compressor.close();
        std::cout << "7z single file compression completed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "7z single file compression failed: " << e.what() << std::endl;
        throw;
    }

    std::cout << "\nListing entries in single file 7z:" << std::endl;
    Compression::Core::SevenZip::SzDecompressor decompressor;
    try {
        decompressor.open(szPath);
        std::vector<std::string> entries = decompressor.listEntries();
        for (const auto& entry : entries) {
            std::cout << "  - " << entry << std::endl;
        }
        decompressor.close();
    } catch (const std::exception& e) {
        std::cerr << "7z listing failed: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Compression Library Test Suite     " << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\n=== Library Capabilities ===" << std::endl;
    std::cout << "ZLIB available: " << (Compression::Utils::ZlibUtils::isZlibAvailable() ? "Yes" : "No") << std::endl;
    std::cout << "LZMA available: " << (Compression::Utils::LZMAUtils::isLzmaAvailable() ? "Yes" : "No") << std::endl;

    std::string testDir = "./test_files";
    std::string zipPath = "./test_output.zip";
    std::string singleZipPath = "./single_file_test.zip";
    std::string zipOutputDir = "./extracted_files_zip";
    
    std::string szPath = "./test_output.7z";
    std::string singleSzPath = "./single_file_test.7z";
    std::string szOutputDir = "./extracted_files_7z";

    try {
        createTestFiles(testDir);

        testZipCompression(testDir, zipPath);
        testZipListing(zipPath);
        testZipDecompression(zipPath, zipOutputDir);

        test7zCompression(testDir, szPath);
        test7zListing(szPath);
        test7zDecompression(szPath, szOutputDir);

        test7zSingleFileOperations(testDir, singleSzPath);

        std::cout << "\n========================================" << std::endl;
        std::cout << "      All tests completed successfully!   " << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\nGenerated files:" << std::endl;
        std::cout << "  ZIP: " << zipPath << std::endl;
        std::cout << "  7z:  " << szPath << std::endl;
        std::cout << "  Single ZIP: " << singleZipPath << std::endl;
        std::cout << "  Single 7z:  " << singleSzPath << std::endl;
        std::cout << "  Extracted ZIP: " << zipOutputDir << "/" << std::endl;
        std::cout << "  Extracted 7z:  " << szOutputDir << "/" << std::endl;
        std::cout << "  Test files: " << testDir << "/" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
