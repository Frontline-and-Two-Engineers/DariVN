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

#ifndef DARIVN_VFS_HPP
#define DARIVN_VFS_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

#include "Archive.hpp"

namespace DariVN {


class VFS {
public:
    VFS() = delete;

    // Инициализация VFS: парсинг аргументов запуска (--dev) и автоматический поиск pak/папок
    static void init(int argc = 0, char* argv[] = nullptr);
    static void shutdown();

    // Режим разработчика (включается флагом --dev в командной строке)
    static void setDevMode(bool devMode);
    static bool isDevMode();

    // Монтирование .pak архива
    static bool mountArchive(const std::string& pakPath, const std::string& key = "");

    // Добавление физической папки для поиска
    static void addSearchPath(const std::string& dirPath);

    // Чтение файлов
    static std::vector<uint8_t> readFile(const std::string& path);
    static std::string readTextFile(const std::string& path);
    static bool exists(const std::string& path);

    // Разрешает путь: если файл доступен на физическом диске — возвращает путь к нему,
    // если только в архиве — возвращает виртуальный путь
    static std::string resolveFilePath(const std::string& path);

    // Нормализация пути: замена обратных слэшей, удаление лишних ./
    static std::string normalize(const std::string& path);

private:
    static bool readFromDisk(const std::string& diskPath, std::vector<uint8_t>& outData);

    static inline bool s_devMode = false;
    static inline std::vector<std::unique_ptr<ArchiveReader>> s_archives;
    static inline std::vector<std::string> s_searchPaths;
};

} // namespace DariVN

#endif // DARIVN_VFS_HPP
