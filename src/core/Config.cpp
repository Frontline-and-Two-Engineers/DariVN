#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <filesystem>

std::unordered_map<std::string, std::string> Config::s_entries;
std::unordered_map<std::string, std::string> Config::s_baseEntries;
std::unordered_map<std::string, std::string> Config::s_userOverrides;
std::string Config::s_userSettingsPath = "saves/settings.vn";

static std::string trim(std::string_view s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return std::string(s.substr(start, end - start + 1));
}

static std::string unquote(std::string_view s) {
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        return std::string(s.substr(1, s.size() - 2));
    }
    return std::string(s);
}

bool Config::load(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[Config] Could not open config file: " << filePath << std::endl;
        return false;
    }

    std::string line;
    int loadedCount = 0;

    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.starts_with('#') || trimmed.starts_with("//")) {
            continue;
        }

        size_t sepPos = trimmed.find('=');
        if (sepPos == std::string::npos) {
            sepPos = trimmed.find(':');
        }

        if (sepPos != std::string::npos) {
            std::string key = trim(trimmed.substr(0, sepPos));
            std::string val = unquote(trim(trimmed.substr(sepPos + 1)));

            if (!key.empty()) {
                s_entries[key] = val;
                loadedCount++;
            }
        }
    }

    s_baseEntries = s_entries;
    std::cout << "[Config] Successfully loaded " << loadedCount << " settings from " << filePath << std::endl;
    return true;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    auto it = s_entries.find(key);
    if (it != s_entries.end()) {
        return it->second;
    }
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) {
    auto it = s_entries.find(key);
    if (it != s_entries.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {}
    }
    return defaultValue;
}

float Config::getFloat(const std::string& key, float defaultValue) {
    auto it = s_entries.find(key);
    if (it != s_entries.end()) {
        try {
            return std::stof(it->second);
        } catch (...) {}
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    auto it = s_entries.find(key);
    if (it != s_entries.end()) {
        std::string val = it->second;
        std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c) { return std::tolower(c); });
        if (val == "true" || val == "1" || val == "yes" || val == "on") return true;
        if (val == "false" || val == "0" || val == "no" || val == "off") return false;
    }
    return defaultValue;
}

bool Config::has(const std::string& key) {
    return s_entries.find(key) != s_entries.end();
}

std::vector<std::pair<std::string, std::string>> Config::getEntriesWithPrefix(const std::string& prefix) {
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& [k, v] : s_entries) {
        if (k.starts_with(prefix)) {
            result.emplace_back(k, v);
        }
    }
    return result;
}

void Config::set(const std::string& key, const std::string& value) {
    s_entries[key] = value;
}

void Config::clear() {
    s_entries.clear();
    s_baseEntries.clear();
    s_userOverrides.clear();
}

bool Config::loadUserSettings(const std::string& filePath) {
    std::string path = filePath.empty() ? s_userSettingsPath : filePath;
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    int loadedCount = 0;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.starts_with('#') || trimmed.starts_with("//")) {
            continue;
        }

        size_t sepPos = trimmed.find('=');
        if (sepPos == std::string::npos) {
            sepPos = trimmed.find(':');
        }

        if (sepPos != std::string::npos) {
            std::string key = trim(trimmed.substr(0, sepPos));
            std::string val = unquote(trim(trimmed.substr(sepPos + 1)));

            if (!key.empty()) {
                s_userOverrides[key] = val;
                s_entries[key] = val; // Приоритетная перезапись базового конфига
                loadedCount++;
            }
        }
    }

    std::cout << "[Config] Successfully loaded " << loadedCount << " user overrides from " << path << std::endl;
    return true;
}

bool Config::saveUserSettings(const std::string& filePath) {
    std::string path = filePath.empty() ? s_userSettingsPath : filePath;
    try {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (const std::exception& e) {
        std::cerr << "[Config] Failed to create directories for: " << path << " (" << e.what() << ")" << std::endl;
        return false;
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Config] Failed to open user settings file for writing: " << path << std::endl;
        return false;
    }

    file << "# ==============================================================================\n";
    file << "# Пользовательские настройки визуальной новеллы DariVN (settings.vn)\n";
    file << "# Переопределяют базовые параметры из config.vn\n";
    file << "# ==============================================================================\n\n";

    // Сортируем ключи для аккуратного и детерминированного вывода
    std::vector<std::string> keys;
    for (const auto& [k, v] : s_userOverrides) {
        keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());

    for (const auto& k : keys) {
        const std::string& val = s_userOverrides[k];
        file << k << " = " << val << "\n";
    }

    file.close();
    std::cout << "[Config] Successfully saved " << keys.size() << " user settings to: " << path << std::endl;
    return true;
}

void Config::setUserSetting(const std::string& key, const std::string& value) {
    s_userOverrides[key] = value;
    s_entries[key] = value;
}

void Config::resetUserSettings() {
    s_userOverrides.clear();
    s_entries = s_baseEntries;

    // Удаляем файл пользовательских настроек с диска, если он существовал
    try {
        if (std::filesystem::exists(s_userSettingsPath)) {
            std::filesystem::remove(s_userSettingsPath);
            std::cout << "[Config] User settings file removed on reset: " << s_userSettingsPath << std::endl;
        }
    } catch (...) {}
}

std::string Config::getUserSettingsPath() {
    return s_userSettingsPath;
}

void Config::setUserSettingsPath(const std::string& path) {
    s_userSettingsPath = path;
}
