/*
 * DariVN - Visual Novel Engine
 * Asset Packer Utility (darivn-packer)
 *
 * Copyright (C) 2026 Arsenii Soloviov <arsenii.soloviov.02@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "vfs/Archive.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

static void printUsage(const char* progName) {
    std::cout << "DariVN Asset Packer (v" << DariVN::ARCHIVE_VERSION << ")\n\n"
              << "Usage:\n"
              << "  " << progName << " pack   <input_dir> <output.pak> [options]\n"
              << "  " << progName << " unpack <input.pak> <output_dir> [options]\n"
              << "  " << progName << " list   <input.pak> [options]\n"
              << "  " << progName << " verify <input.pak> [options]\n\n"
              << "Options:\n"
              << "  --key <str>, -k <str>      Encryption key for package TOC and payloads\n"
              << "  --no-compress              Store files without compression\n"
              << "  --prefix <str>             Prefix for paths inside archive (e.g. 'assets')\n"
              << "  --verbose, -v              Display detailed processing logs\n"
              << std::endl;
}

static std::string formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIdx = 0;
    double d = static_cast<double>(bytes);
    while (d >= 1024.0 && unitIdx < 3) {
        d /= 1024.0;
        unitIdx++;
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(unitIdx > 0 ? 2 : 0) << d << " " << units[unitIdx];
    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];
    std::string key = "";
    bool compress = true;
    bool verbose = false;
    std::string prefix = "";
    std::vector<std::string> positionalArgs;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--key" || arg == "-k") {
            if (i + 1 < argc) key = argv[++i];
        } else if (arg == "--no-compress") {
            compress = false;
        } else if (arg == "--prefix") {
            if (i + 1 < argc) prefix = argv[++i];
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg.starts_with("-")) {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        } else {
            positionalArgs.push_back(arg);
        }
    }

    auto startTime = std::chrono::steady_clock::now();

    if (command == "pack") {
        if (positionalArgs.size() < 2) {
            std::cerr << "Error: pack requires <input_dir> and <output.pak>\n";
            return 1;
        }
        std::string inputDir = positionalArgs[0];
        std::string outputPak = positionalArgs[1];

        if (!fs::exists(inputDir) || !fs::is_directory(inputDir)) {
            std::cerr << "Error: Directory not found: " << inputDir << "\n";
            return 1;
        }

        // Если prefix не задан явно, но папка называется "assets", ставим префикс "assets"
        if (prefix.empty()) {
            std::string cleanDir = inputDir;
            while (cleanDir.ends_with('/') || cleanDir.ends_with('\\')) cleanDir.pop_back();
            if (cleanDir == "assets" || cleanDir.ends_with("/assets") || cleanDir.ends_with("\\assets")) {
                prefix = "assets";
            }
        }

        std::cout << "[PACK] Packing directory '" << inputDir << "' into '" << outputPak << "'...\n";

        if (!key.empty()) {
            std::cout << "[PACK] Encryption enabled with custom key.\n";
        }

        DariVN::ArchiveWriter writer;
        if (!key.empty()) {
            writer.setKey(key);
        }

        DariVN::ArchiveWriter::AddOptions opts;
        opts.compress = compress;
        opts.encrypt = !key.empty();

        if (!writer.addDirectory(inputDir, prefix, opts)) {
            std::cerr << "Error: Failed to process directory.\n";
            return 1;
        }

        if (!writer.writeToFile(outputPak)) {
            std::cerr << "Error: Failed to write archive to '" << outputPak << "'\n";
            return 1;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        uint64_t orig = writer.getTotalOriginalSize();
        uint64_t comp = writer.getTotalCompressedSize();
        double ratio = (orig > 0) ? (100.0 * (1.0 - static_cast<double>(comp) / orig)) : 0.0;

        std::cout << "[PACK] Success! Packed " << writer.getFileCount() << " files in " << elapsed << " ms.\n"
                  << "       Original Size:   " << formatBytes(orig) << " (" << orig << " bytes)\n"
                  << "       Compressed Size: " << formatBytes(comp) << " (" << comp << " bytes)\n"
                  << "       Space Saved:     " << std::fixed << std::setprecision(1) << ratio << "%\n";

    } else if (command == "list") {
        if (positionalArgs.empty()) {
            std::cerr << "Error: list requires <input.pak>\n";
            return 1;
        }
        std::string inputPak = positionalArgs[0];

        DariVN::ArchiveReader reader;
        if (!reader.open(inputPak, key)) {
            std::cerr << "Error: Failed to open archive: " << inputPak << "\n";
            return 1;
        }

        std::cout << "Archive: " << inputPak << "\n"
                  << "Files:   " << reader.getEntries().size() << "\n"
                  << "--------------------------------------------------------------------------------\n"
                  << std::left << std::setw(42) << "Path"
                  << std::right << std::setw(12) << "Orig Size"
                  << std::setw(12) << "Comp Size"
                  << std::setw(8) << "Method"
                  << std::setw(6) << "Enc"
                  << "\n--------------------------------------------------------------------------------\n";

        uint64_t totalOrig = 0, totalComp = 0;
        for (const auto& entry : reader.getEntries()) {
            totalOrig += entry.uncompressedSize;
            totalComp += entry.compressedSize;

            std::string method = (entry.compression == DariVN::COMPRESSION_DEFLATE) ? "DEFLATE" : "STORE";
            std::string enc = (entry.flags & DariVN::FLAG_ENCRYPTED) ? "YES" : "NO";

            std::cout << std::left << std::setw(42) << (entry.path.size() > 41 ? entry.path.substr(0, 38) + "..." : entry.path)
                      << std::right << std::setw(12) << formatBytes(entry.uncompressedSize)
                      << std::setw(12) << formatBytes(entry.compressedSize)
                      << std::setw(8) << method
                      << std::setw(6) << enc
                      << "\n";
        }

        std::cout << "--------------------------------------------------------------------------------\n"
                  << "Total: " << formatBytes(totalOrig) << " -> " << formatBytes(totalComp) << "\n";

    } else if (command == "verify") {
        if (positionalArgs.empty()) {
            std::cerr << "Error: verify requires <input.pak>\n";
            return 1;
        }
        std::string inputPak = positionalArgs[0];

        DariVN::ArchiveReader reader;
        if (!reader.open(inputPak, key)) {
            std::cerr << "Error: Failed to open archive: " << inputPak << "\n";
            return 1;
        }

        std::cout << "[VERIFY] Verifying " << reader.getEntries().size() << " files in '" << inputPak << "'...\n";
        std::string errorMsg;
        if (reader.verify(errorMsg)) {
            std::cout << "[VERIFY] OK! All files and CRC32 checksums are valid.\n";
        } else {
            std::cerr << "[VERIFY] FAILED: " << errorMsg << "\n";
            return 1;
        }

    } else if (command == "unpack") {
        if (positionalArgs.size() < 2) {
            std::cerr << "Error: unpack requires <input.pak> and <output_dir>\n";
            return 1;
        }
        std::string inputPak = positionalArgs[0];
        std::string outputDir = positionalArgs[1];

        DariVN::ArchiveReader reader;
        if (!reader.open(inputPak, key)) {
            std::cerr << "Error: Failed to open archive: " << inputPak << "\n";
            return 1;
        }

        std::cout << "[UNPACK] Unpacking to '" << outputDir << "'...\n";
        fs::create_directories(outputDir);

        size_t extracted = 0;
        for (const auto& entry : reader.getEntries()) {
            std::vector<uint8_t> buffer;
            if (!reader.readFile(entry.path, buffer)) {
                std::cerr << "Error extracting: " << entry.path << "\n";
                return 1;
            }

            fs::path dest = fs::path(outputDir) / entry.path;
            fs::create_directories(dest.parent_path());

            std::ofstream out(dest, std::ios::binary);
            if (!out.is_open()) {
                std::cerr << "Cannot write file: " << dest << "\n";
                return 1;
            }

            if (!buffer.empty()) {
                out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            }

            if (verbose) {
                std::cout << "Extracted: " << entry.path << " (" << buffer.size() << " bytes)\n";
            }
            extracted++;
        }

        std::cout << "[UNPACK] Successfully extracted " << extracted << " files.\n";

    } else {
        std::cerr << "Unknown command: " << command << "\n";
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
