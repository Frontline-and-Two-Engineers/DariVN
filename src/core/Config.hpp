#pragma once

#ifndef DARIVN_CONFIG_HPP
#define DARIVN_CONFIG_HPP

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <utility>

class Config {
public:
    // Загрузка конфигурационного файла (например, config.vn или assets/config.vn)
    static bool load(const std::string& filePath);

    // Типизированный доступ к значениям
    static std::string getString(const std::string& key, const std::string& defaultValue = "");
    static int getInt(const std::string& key, int defaultValue = 0);
    static float getFloat(const std::string& key, float defaultValue = 0.0f);
    static bool getBool(const std::string& key, bool defaultValue = false);

    // Получение всех записей с заданным префиксом (например, "locale.")
    static std::vector<std::pair<std::string, std::string>> getEntriesWithPrefix(const std::string& prefix);

    // Пользовательские настройки (имеют приоритет над базовым файлом конфигурации)
    static bool loadUserSettings(const std::string& filePath = "");
    static bool saveUserSettings(const std::string& filePath = "");
    static void setUserSetting(const std::string& key, const std::string& value);
    static void resetUserSettings();
    static std::string getUserSettingsPath();
    static void setUserSettingsPath(const std::string& path);

    // Проверка наличия и установка параметров
    static bool has(const std::string& key);
    static void set(const std::string& key, const std::string& value);
    static void clear();

private:
    static std::unordered_map<std::string, std::string> s_entries;
    static std::unordered_map<std::string, std::string> s_baseEntries;
    static std::unordered_map<std::string, std::string> s_userOverrides;
    static std::string s_userSettingsPath;
};

#endif //DARIVN_CONFIG_HPP
