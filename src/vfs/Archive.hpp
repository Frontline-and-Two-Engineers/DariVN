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

#pragma once

#ifndef DARIVN_ARCHIVE_HPP
#define DARIVN_ARCHIVE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <fstream>

namespace DariVN {

constexpr char ARCHIVE_MAGIC[4] = {'D', 'V', 'N', 'P'};
constexpr uint32_t ARCHIVE_VERSION = 1;

enum ArchiveCompression : uint16_t {
    COMPRESSION_STORE = 0,
    COMPRESSION_DEFLATE = 1
};

enum ArchiveFlags : uint32_t {
    FLAG_NONE = 0,
    FLAG_ENCRYPTED = 1 << 0
};

#pragma pack(push, 1)
struct ArchiveHeader {
    char magic[4] = {'D', 'V', 'N', 'P'};
    uint32_t version = ARCHIVE_VERSION;
    uint32_t flags = 0;
    uint32_t fileCount = 0;
    uint64_t tocOffset = 0;
    uint64_t tocCompressedSize = 0;
    uint64_t tocUncompressedSize = 0;
    uint8_t reserved[24] = {0};
};
#pragma pack(pop)

struct ArchiveEntry {
    std::string path;              // Нормализованный путь (например: "assets/scripts/demo.vn")
    uint64_t offset = 0;           // Смещение в файле архива
    uint64_t compressedSize = 0;   // Размер в архиве
    uint64_t uncompressedSize = 0; // Исходный размер
    uint32_t crc32 = 0;            // Контрольная сумма исходных данных
    uint16_t compression = COMPRESSION_STORE;
    uint16_t flags = 0;
};

// Чтение архива
class ArchiveReader {
public:
    ArchiveReader();
    ~ArchiveReader();

    bool open(const std::string& archivePath, const std::string& key = "");
    void close();

    bool isOpen() const { return m_isOpen; }
    const std::string& getPath() const { return m_path; }
    const std::vector<ArchiveEntry>& getEntries() const { return m_entryList; }
    const ArchiveHeader& getHeader() const { return m_header; }

    bool hasFile(const std::string& normalizedPath) const;
    const ArchiveEntry* getEntry(const std::string& normalizedPath) const;

    bool readFile(const std::string& normalizedPath, std::vector<uint8_t>& outBuffer);
    bool verify(std::string& outErrorMessage);

private:
    std::string m_path;
    std::string m_key;
    bool m_isOpen = false;
    ArchiveHeader m_header{};
    std::vector<ArchiveEntry> m_entryList;
    std::unordered_map<std::string, size_t> m_entryMap;
    mutable std::ifstream m_file;
};

struct ArchiveAddOptions {
    bool compress = true;
    bool encrypt = false;
};

// Запись архива
class ArchiveWriter {
public:
    using AddOptions = ArchiveAddOptions;

    ArchiveWriter();
    ~ArchiveWriter();

    void setKey(const std::string& key) { m_key = key; }

    // Добавляет файл из памяти
    bool addFileFromMemory(const std::string& normalizedPath, 
                          const uint8_t* data, 
                          size_t size, 
                          const ArchiveAddOptions& options = {});

    // Добавляет файл с диска
    bool addFileFromDisk(const std::string& diskFilePath, 
                        const std::string& archiveRelativePath, 
                        const ArchiveAddOptions& options = {});

    // Рекурсивно упаковывает директорию
    bool addDirectory(const std::string& diskDirPath, 
                     const std::string& archivePrefix = "", 
                     const ArchiveAddOptions& options = {});

    // Финализирует и записывает архив на диск
    bool writeToFile(const std::string& outputArchivePath);


    size_t getFileCount() const { return m_entries.size(); }
    uint64_t getTotalOriginalSize() const { return m_totalOriginalSize; }
    uint64_t getTotalCompressedSize() const { return m_totalCompressedSize; }

private:
    struct StagedFile {
        ArchiveEntry entry;
        std::vector<uint8_t> payload;
    };

    std::string m_key;
    std::vector<StagedFile> m_entries;
    uint64_t m_totalOriginalSize = 0;
    uint64_t m_totalCompressedSize = 0;
};

// Утилиты шифрования и хэширования
uint32_t calculateCRC32(const uint8_t* data, size_t size);
void applyXorCipher(uint8_t* data, size_t size, const std::string& key);

} // namespace DariVN

#endif // DARIVN_ARCHIVE_HPP
