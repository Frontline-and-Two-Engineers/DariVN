/*
 * DariVN - Visual Novel Engine
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

#include "Archive.hpp"
#include <zlib.h>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <algorithm>

namespace DariVN {

static std::string normalizeArchiveItemPath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    while (path.starts_with("./")) {
        path = path.substr(2);
    }
    while (path.starts_with("/")) {
        path = path.substr(1);
    }
    return path;
}

static bool isAlreadyCompressedExtension(const std::string& path) {
    static const std::vector<std::string> compressedExts = {
        ".png", ".jpg", ".jpeg", ".webp", ".mp3", ".ogg", ".wav", ".flac", ".zip", ".pak"
    };
    auto dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) return false;
    std::string ext = path.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    for (const auto& e : compressedExts) {
        if (ext == e) return true;
    }
    return false;
}

uint32_t calculateCRC32(const uint8_t* data, size_t size) {
    if (!data || size == 0) return 0;
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, data, static_cast<uInt>(size));
    return static_cast<uint32_t>(crc);
}

void applyXorCipher(uint8_t* data, size_t size, const std::string& key) {
    if (!data || size == 0 || key.empty()) return;
    uint32_t seed = 0x811c9dc5;
    for (char c : key) {
        seed = (seed ^ static_cast<uint8_t>(c)) * 0x01000193;
    }
    for (size_t i = 0; i < size; ++i) {
        uint8_t k = static_cast<uint8_t>(key[(i + seed) % key.size()]);
        data[i] ^= (k ^ static_cast<uint8_t>((i * 31) & 0xFF));
    }
}

// ============================================================================
// ArchiveReader
// ============================================================================

ArchiveReader::ArchiveReader() = default;

ArchiveReader::~ArchiveReader() {
    close();
}

void ArchiveReader::close() {
    if (m_file.is_open()) {
        m_file.close();
    }
    m_isOpen = false;
    m_path.clear();
    m_key.clear();
    m_entryList.clear();
    m_entryMap.clear();
}

bool ArchiveReader::open(const std::string& archivePath, const std::string& key) {
    close();

    m_file.open(archivePath, std::ios::binary);
    if (!m_file.is_open()) {
        return false;
    }

    m_path = archivePath;
    m_key = key;

    // Читаем заголовок
    m_file.read(reinterpret_cast<char*>(&m_header), sizeof(ArchiveHeader));
    if (!m_file || std::memcmp(m_header.magic, ARCHIVE_MAGIC, 4) != 0) {
        close();
        return false;
    }

    if (m_header.version != ARCHIVE_VERSION) {
        std::cerr << "[ArchiveReader] Unsupported version " << m_header.version 
                  << " in archive: " << archivePath << std::endl;
        close();
        return false;
    }

    // Переходим к оглавлению (TOC)
    m_file.seekg(m_header.tocOffset, std::ios::beg);
    if (!m_file) {
        close();
        return false;
    }

    std::vector<uint8_t> tocData(m_header.tocCompressedSize);
    m_file.read(reinterpret_cast<char*>(tocData.data()), m_header.tocCompressedSize);
    if (!m_file) {
        close();
        return false;
    }

    // Дешифровка TOC при необходимости
    if (m_header.flags & FLAG_ENCRYPTED) {
        applyXorCipher(tocData.data(), tocData.size(), m_key);
    }

    // Распаковка TOC при необходимости
    std::vector<uint8_t> uncompressedTOC;
    if (m_header.tocCompressedSize != m_header.tocUncompressedSize) {
        uncompressedTOC.resize(m_header.tocUncompressedSize);
        uLongf destLen = static_cast<uLongf>(m_header.tocUncompressedSize);
        int res = uncompress(uncompressedTOC.data(), &destLen, tocData.data(), static_cast<uLong>(tocData.size()));
        if (res != Z_OK) {
            std::cerr << "[ArchiveReader] Failed to decompress TOC (corrupted archive or bad key)" << std::endl;
            close();
            return false;
        }
    } else {
        uncompressedTOC = std::move(tocData);
    }

    // Парсим записи оглавления
    m_entryList.reserve(m_header.fileCount);
    size_t cursor = 0;
    const uint8_t* ptr = uncompressedTOC.data();
    size_t totalBytes = uncompressedTOC.size();

    for (uint32_t i = 0; i < m_header.fileCount; ++i) {
        if (cursor + sizeof(uint16_t) > totalBytes) break;

        uint16_t pathLen = 0;
        std::memcpy(&pathLen, ptr + cursor, sizeof(uint16_t));
        cursor += sizeof(uint16_t);

        if (cursor + pathLen > totalBytes) break;
        std::string entryPath(reinterpret_cast<const char*>(ptr + cursor), pathLen);
        cursor += pathLen;

        struct EntryBinaryData {
            uint64_t offset;
            uint64_t compressedSize;
            uint64_t uncompressedSize;
            uint32_t crc32;
            uint16_t compression;
            uint16_t flags;
        };

        if (cursor + sizeof(EntryBinaryData) > totalBytes) break;
        EntryBinaryData binData{};
        std::memcpy(&binData, ptr + cursor, sizeof(EntryBinaryData));
        cursor += sizeof(EntryBinaryData);

        ArchiveEntry entry;
        entry.path = normalizeArchiveItemPath(entryPath);
        entry.offset = binData.offset;
        entry.compressedSize = binData.compressedSize;
        entry.uncompressedSize = binData.uncompressedSize;
        entry.crc32 = binData.crc32;
        entry.compression = binData.compression;
        entry.flags = binData.flags;

        size_t idx = m_entryList.size();
        m_entryMap[entry.path] = idx;
        m_entryList.push_back(std::move(entry));
    }

    m_isOpen = true;
    return true;
}

bool ArchiveReader::hasFile(const std::string& normalizedPath) const {
    if (!m_isOpen) return false;
    std::string norm = normalizeArchiveItemPath(normalizedPath);
    return m_entryMap.find(norm) != m_entryMap.end();
}

const ArchiveEntry* ArchiveReader::getEntry(const std::string& normalizedPath) const {
    if (!m_isOpen) return nullptr;
    std::string norm = normalizeArchiveItemPath(normalizedPath);
    auto it = m_entryMap.find(norm);
    if (it != m_entryMap.end()) {
        return &m_entryList[it->second];
    }
    return nullptr;
}

bool ArchiveReader::readFile(const std::string& normalizedPath, std::vector<uint8_t>& outBuffer) {
    if (!m_isOpen) return false;

    const ArchiveEntry* entry = getEntry(normalizedPath);
    if (!entry) return false;

    m_file.seekg(entry->offset, std::ios::beg);
    if (!m_file) return false;

    std::vector<uint8_t> compBuffer(entry->compressedSize);
    m_file.read(reinterpret_cast<char*>(compBuffer.data()), entry->compressedSize);
    if (!m_file) return false;

    // Дешифрование пейлоада
    if (entry->flags & FLAG_ENCRYPTED) {
        applyXorCipher(compBuffer.data(), compBuffer.size(), m_key);
    }

    // Декомпрессия
    if (entry->compression == COMPRESSION_DEFLATE) {
        outBuffer.resize(entry->uncompressedSize);
        uLongf destLen = static_cast<uLongf>(entry->uncompressedSize);
        int res = uncompress(outBuffer.data(), &destLen, compBuffer.data(), static_cast<uLong>(compBuffer.size()));
        if (res != Z_OK) {
            std::cerr << "[ArchiveReader] Error decompressing file: " << normalizedPath << std::endl;
            outBuffer.clear();
            return false;
        }
    } else {
        outBuffer = std::move(compBuffer);
    }

    return true;
}

bool ArchiveReader::verify(std::string& outErrorMessage) {
    if (!m_isOpen) {
        outErrorMessage = "Archive is not open";
        return false;
    }

    for (const auto& entry : m_entryList) {
        std::vector<uint8_t> data;
        if (!readFile(entry.path, data)) {
            outErrorMessage = "Failed to read file: " + entry.path;
            return false;
        }
        uint32_t calcCrc = calculateCRC32(data.data(), data.size());
        if (calcCrc != entry.crc32) {
            outErrorMessage = "CRC32 mismatch for: " + entry.path + 
                             " (expected " + std::to_string(entry.crc32) + 
                             ", got " + std::to_string(calcCrc) + ")";
            return false;
        }
    }
    return true;
}

// ============================================================================
// ArchiveWriter
// ============================================================================

ArchiveWriter::ArchiveWriter() = default;
ArchiveWriter::~ArchiveWriter() = default;

bool ArchiveWriter::addFileFromMemory(const std::string& normalizedPath, 
                                     const uint8_t* data, 
                                     size_t size, 
                                     const AddOptions& options) {
    if (!data && size > 0) return false;

    std::string norm = normalizeArchiveItemPath(normalizedPath);
    if (norm.empty()) return false;

    StagedFile staged;
    staged.entry.path = norm;
    staged.entry.uncompressedSize = size;
    staged.entry.crc32 = calculateCRC32(data, size);

    bool shouldCompress = options.compress && !isAlreadyCompressedExtension(norm) && (size > 64);
    std::vector<uint8_t> processedData;

    if (shouldCompress) {
        uLong maxCompSize = compressBound(static_cast<uLong>(size));
        processedData.resize(maxCompSize);
        uLongf destLen = maxCompSize;

        int res = compress(processedData.data(), &destLen, data, static_cast<uLong>(size));
        if (res == Z_OK && destLen < size) {
            processedData.resize(destLen);
            staged.entry.compression = COMPRESSION_DEFLATE;
        } else {
            // Если сжатие не уменьшило размер
            processedData.assign(data, data + size);
            staged.entry.compression = COMPRESSION_STORE;
        }
    } else {
        processedData.assign(data, data + size);
        staged.entry.compression = COMPRESSION_STORE;
    }

    if (options.encrypt && !m_key.empty()) {
        applyXorCipher(processedData.data(), processedData.size(), m_key);
        staged.entry.flags |= FLAG_ENCRYPTED;
    }

    staged.entry.compressedSize = processedData.size();
    staged.payload = std::move(processedData);

    m_totalOriginalSize += staged.entry.uncompressedSize;
    m_totalCompressedSize += staged.entry.compressedSize;

    m_entries.push_back(std::move(staged));
    return true;
}

bool ArchiveWriter::addFileFromDisk(const std::string& diskFilePath, 
                                   const std::string& archiveRelativePath, 
                                   const AddOptions& options) {
    std::ifstream file(diskFilePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[ArchiveWriter] Cannot open disk file: " << diskFilePath << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (size > 0 && !file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "[ArchiveWriter] Failed to read disk file: " << diskFilePath << std::endl;
        return false;
    }

    return addFileFromMemory(archiveRelativePath, buffer.data(), buffer.size(), options);
}

bool ArchiveWriter::addDirectory(const std::string& diskDirPath, 
                                const std::string& archivePrefix, 
                                const AddOptions& options) {
    namespace fs = std::filesystem;
    if (!fs::exists(diskDirPath) || !fs::is_directory(diskDirPath)) {
        std::cerr << "[ArchiveWriter] Directory does not exist: " << diskDirPath << std::endl;
        return false;
    }

    for (const auto& entry : fs::recursive_directory_iterator(diskDirPath)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        if (filename == ".DS_Store" || filename.starts_with(".")) continue;

        auto rel = fs::relative(entry.path(), diskDirPath);
        std::string archivePath = archivePrefix.empty() ? rel.string() : (fs::path(archivePrefix) / rel).string();

        if (!addFileFromDisk(entry.path().string(), archivePath, options)) {
            std::cerr << "[ArchiveWriter] Failed adding: " << entry.path() << std::endl;
            return false;
        }
    }

    return true;
}

bool ArchiveWriter::writeToFile(const std::string& outputArchivePath) {
    std::ofstream out(outputArchivePath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "[ArchiveWriter] Cannot create archive: " << outputArchivePath << std::endl;
        return false;
    }

    ArchiveHeader header{};
    std::memcpy(header.magic, ARCHIVE_MAGIC, 4);
    header.version = ARCHIVE_VERSION;
    header.fileCount = static_cast<uint32_t>(m_entries.size());
    if (!m_key.empty()) {
        header.flags |= FLAG_ENCRYPTED;
    }

    // Резервируем место под заголовок в начале файла
    out.write(reinterpret_cast<const char*>(&header), sizeof(ArchiveHeader));

    // Записываем полезные данные файлов и рассчитываем их смещения
    for (auto& staged : m_entries) {
        staged.entry.offset = static_cast<uint64_t>(out.tellp());
        if (!staged.payload.empty()) {
            out.write(reinterpret_cast<const char*>(staged.payload.data()), staged.payload.size());
        }
    }

    // Формируем оглавление (TOC)
    std::vector<uint8_t> uncompressedTOC;
    for (const auto& staged : m_entries) {
        const auto& e = staged.entry;
        uint16_t pathLen = static_cast<uint16_t>(e.path.size());

        size_t prevSize = uncompressedTOC.size();
        uncompressedTOC.resize(prevSize + sizeof(uint16_t) + pathLen);
        std::memcpy(uncompressedTOC.data() + prevSize, &pathLen, sizeof(uint16_t));
        std::memcpy(uncompressedTOC.data() + prevSize + sizeof(uint16_t), e.path.data(), pathLen);

        struct EntryBinaryData {
            uint64_t offset;
            uint64_t compressedSize;
            uint64_t uncompressedSize;
            uint32_t crc32;
            uint16_t compression;
            uint16_t flags;
        } binData{e.offset, e.compressedSize, e.uncompressedSize, e.crc32, e.compression, e.flags};

        size_t binPos = uncompressedTOC.size();
        uncompressedTOC.resize(binPos + sizeof(EntryBinaryData));
        std::memcpy(uncompressedTOC.data() + binPos, &binData, sizeof(EntryBinaryData));
    }

    header.tocUncompressedSize = uncompressedTOC.size();

    // Сжимаем TOC
    std::vector<uint8_t> finalTOC;
    uLong maxTOCComp = compressBound(static_cast<uLong>(uncompressedTOC.size()));
    finalTOC.resize(maxTOCComp);
    uLongf destLen = maxTOCComp;

    int res = compress(finalTOC.data(), &destLen, uncompressedTOC.data(), static_cast<uLong>(uncompressedTOC.size()));
    if (res == Z_OK && destLen < uncompressedTOC.size()) {
        finalTOC.resize(destLen);
    } else {
        finalTOC = std::move(uncompressedTOC);
    }

    header.tocCompressedSize = finalTOC.size();

    // Шифруем TOC если задан ключ
    if (!m_key.empty()) {
        applyXorCipher(finalTOC.data(), finalTOC.size(), m_key);
    }

    header.tocOffset = static_cast<uint64_t>(out.tellp());
    out.write(reinterpret_cast<const char*>(finalTOC.data()), finalTOC.size());

    // Возвращаемся и перезаписываем заголовок с актуальными смещениями
    out.seekp(0, std::ios::beg);
    out.write(reinterpret_cast<const char*>(&header), sizeof(ArchiveHeader));

    return true;
}

} // namespace DariVN
