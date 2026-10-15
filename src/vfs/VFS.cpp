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

#include "VFS.hpp"
#include "Archive.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace DariVN {

namespace fs = std::filesystem;

static fs::path getExecutableDir() {
    static fs::path s_exeDir = []() {
        try {
#if defined(__APPLE__)
            char path[1024];
            uint32_t size = sizeof(path);
            if (_NSGetExecutablePath(path, &size) == 0) {
                return fs::canonical(path).parent_path();
            }
#elif defined(_WIN32)
            wchar_t path[MAX_PATH];
            if (GetModuleFileNameW(NULL, path, MAX_PATH) > 0) {
                return fs::canonical(path).parent_path();
            }
#else
            char path[1024];
            ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
            if (count != -1) {
                path[count] = '\0';
                return fs::canonical(path).parent_path();
            }
#endif
        } catch (...) {}
        return fs::current_path();
    }();
    return s_exeDir;
}

std::string VFS::normalize(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    while (result.starts_with("./") || result.starts_with("../")) {
        if (result.starts_with("./")) result = result.substr(2);
        else if (result.starts_with("../")) result = result.substr(3);
    }
    while (result.starts_with("/")) {
        result = result.substr(1);
    }
    return result;
}

void VFS::init(int argc, char* argv[]) {
    shutdown();

    // 1. Проверяем аргументы командной строки
    for (int i = 1; i < argc; ++i) {
        if (!argv[i]) continue;
        std::string arg = argv[i];
        if (arg == "--dev" || arg == "-d" || arg == "--developer") {
            s_devMode = true;
        }
    }

    // 2. Настраиваем стандартные пути поиска на физическом диске
    addSearchPath(".");
    addSearchPath("assets");

    auto exeDir = getExecutableDir();
    addSearchPath(exeDir.string());
    addSearchPath((exeDir / "assets").string());
    addSearchPath(exeDir.parent_path().string());
    addSearchPath((exeDir.parent_path() / "assets").string());

    if (s_devMode) {
        std::cout << "[VFS] ----------------------------------------------------" << std::endl;
        std::cout << "[VFS] [!] DEVELOPER MODE ACTIVE (--dev)" << std::endl;
        std::cout << "[VFS] Loading raw files directly from disk." << std::endl;
        std::cout << "[VFS] ----------------------------------------------------" << std::endl;
    } else {
        std::cout << "[VFS] Running in Standard/Release mode." << std::endl;
    }

    // 3. Пытаемся найти и смонтировать архивы game.pak
    std::vector<fs::path> candidatePaks = {
        "game.pak",
        "assets.pak",
        exeDir / "game.pak",
        exeDir / "assets.pak",
        exeDir.parent_path() / "game.pak",
        exeDir.parent_path() / "assets.pak"
    };

    bool mountedAny = false;
    for (const auto& pak : candidatePaks) {
        if (fs::exists(pak) && fs::is_regular_file(pak)) {
            if (mountArchive(pak.string())) {
                mountedAny = true;
                break; // Монтируем первый найденный основной архив
            }
        }
    }

    if (!mountedAny && !s_devMode) {
        std::cout << "[VFS] Notice: 'game.pak' not found. Falling back to disk assets." << std::endl;
    }
}

void VFS::shutdown() {
    s_archives.clear();
    s_searchPaths.clear();
    s_devMode = false;
}

void VFS::setDevMode(bool devMode) {
    s_devMode = devMode;
}

bool VFS::isDevMode() {
    return s_devMode;
}

bool VFS::mountArchive(const std::string& pakPath, const std::string& key) {
    auto reader = std::make_unique<ArchiveReader>();
    if (!reader->open(pakPath, key)) {
        std::cerr << "[VFS] Failed to mount archive: " << pakPath << std::endl;
        return false;
    }

    std::cout << "[VFS] Mounted archive: " << pakPath 
              << " (" << reader->getEntries().size() << " files)" << std::endl;

    s_archives.push_back(std::move(reader));
    return true;
}

void VFS::addSearchPath(const std::string& dirPath) {
    if (dirPath.empty()) return;
    std::string norm = normalize(dirPath);
    for (const auto& p : s_searchPaths) {
        if (p == dirPath || p == norm) return;
    }
    s_searchPaths.push_back(dirPath);
}

bool VFS::readFromDisk(const std::string& diskPath, std::vector<uint8_t>& outData) {
    std::ifstream file(diskPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    outData.resize(size);
    if (size > 0 && !file.read(reinterpret_cast<char*>(outData.data()), size)) {
        outData.clear();
        return false;
    }
    return true;
}

static std::vector<std::string> getPathVariants(const std::string& rawPath) {
    std::vector<std::string> variants;
    if (rawPath.empty()) return variants;

    std::string norm = VFS::normalize(rawPath);
    if (norm.empty()) return variants;

    variants.push_back(norm);

    // Если в пути есть "assets/", добавляем срез от "assets/" и срез после него
    size_t assetsPos = norm.find("assets/");
    if (assetsPos != std::string::npos) {
        std::string fromAssets = norm.substr(assetsPos);
        if (fromAssets != norm) {
            variants.push_back(fromAssets);
        }
        std::string afterAssets = norm.substr(assetsPos + 7);
        if (!afterAssets.empty()) {
            variants.push_back(afterAssets);
        }
    } else {
        variants.push_back("assets/" + norm);
    }

    return variants;
}

bool VFS::exists(const std::string& path) {
    if (path.empty()) return false;

    auto variants = getPathVariants(path);

    auto checkDisk = [&](const std::string& v) -> bool {
        if (fs::exists(v) && fs::is_regular_file(v)) return true;
        for (const auto& sp : s_searchPaths) {
            auto candidate = fs::path(sp) / v;
            if (fs::exists(candidate) && fs::is_regular_file(candidate)) return true;
        }
        return false;
    };

    auto checkArchives = [&](const std::string& v) -> bool {
        for (auto it = s_archives.rbegin(); it != s_archives.rend(); ++it) {
            if ((*it)->hasFile(v)) return true;
        }
        return false;
    };

    if (s_devMode) {
        for (const auto& v : variants) if (checkDisk(v)) return true;
        for (const auto& v : variants) if (checkArchives(v)) return true;
    } else {
        for (const auto& v : variants) if (checkArchives(v)) return true;
        for (const auto& v : variants) if (checkDisk(v)) return true;
    }

    return false;
}

std::vector<uint8_t> VFS::readFile(const std::string& path) {
    std::vector<uint8_t> buffer;
    if (path.empty()) return buffer;

    auto variants = getPathVariants(path);

    auto tryDisk = [&](const std::string& v) -> bool {
        if (fs::exists(v) && fs::is_regular_file(v)) {
            return readFromDisk(v, buffer);
        }
        for (const auto& sp : s_searchPaths) {
            auto candidate = fs::path(sp) / v;
            if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
                return readFromDisk(candidate.string(), buffer);
            }
        }
        return false;
    };

    auto tryArchives = [&](const std::string& v) -> bool {
        for (auto it = s_archives.rbegin(); it != s_archives.rend(); ++it) {
            if ((*it)->readFile(v, buffer)) {
                return true;
            }
        }
        return false;
    };

    if (s_devMode) {
        for (const auto& v : variants) {
            if (tryDisk(v)) return buffer;
        }
        for (const auto& v : variants) {
            if (tryArchives(v)) return buffer;
        }
    } else {
        for (const auto& v : variants) {
            if (tryArchives(v)) return buffer;
        }
        for (const auto& v : variants) {
            if (tryDisk(v)) return buffer;
        }
    }

    return buffer;
}

std::string VFS::readTextFile(const std::string& path) {
    auto data = readFile(path);
    if (data.empty()) return "";
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

std::string VFS::resolveFilePath(const std::string& path) {
    if (path.empty()) return "";

    auto variants = getPathVariants(path);

    auto findOnDisk = [&](const std::string& v) -> std::string {
        if (fs::exists(v) && fs::is_regular_file(v)) return v;
        for (const auto& sp : s_searchPaths) {
            auto candidate = fs::path(sp) / v;
            if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
                return candidate.string();
            }
        }
        return "";
    };

    if (s_devMode) {
        for (const auto& v : variants) {
            std::string d = findOnDisk(v);
            if (!d.empty()) return d;
        }
        for (const auto& v : variants) {
            for (auto it = s_archives.rbegin(); it != s_archives.rend(); ++it) {
                if ((*it)->hasFile(v)) return v;
            }
        }
    } else {
        for (const auto& v : variants) {
            for (auto it = s_archives.rbegin(); it != s_archives.rend(); ++it) {
                if ((*it)->hasFile(v)) return v;
            }
        }
        for (const auto& v : variants) {
            std::string d = findOnDisk(v);
            if (!d.empty()) return d;
        }
    }

    return path;
}

} // namespace DariVN
