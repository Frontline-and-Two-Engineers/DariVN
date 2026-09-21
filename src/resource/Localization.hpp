#pragma once

#ifndef DARIVN_LOCALIZATION_HPP
#define DARIVN_LOCALIZATION_HPP

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Localization {
public:
    Localization() = delete;

    // Загрузка файла локализации (формат: key = value)
    static bool loadLocale(const std::string& langCode, const std::string& filePath);

    // Смена текущего активного языка ("ru", "en", etc.)
    static bool setLanguage(const std::string& langCode);

    // Установка языка по умолчанию (для фолбэка ненайденных ключей)
    static void setDefaultLanguage(const std::string& langCode);
    static const std::string& getDefaultLanguage();

    // Получение текущего языка
    static const std::string& getCurrentLanguage();

    // Список всех загруженных языков
    static std::vector<std::string> getAvailableLanguages();

    // Проверка наличия ключа в текущем языке (или языке по умолчанию)
    static bool has(std::string_view key);

    // Получение переведенной строки по ключу
    static std::string get(std::string_view key, std::string_view defaultValue = "");

    // Разрешение ключей и инлайн-переводов:
    // 1. Если текст начинается с '@': "@story.intro" -> Localization::get("story.intro")
    // 2. Инлайн-подстановки: "{loc=some.key}" или "@{some.key}" -> Localization::get("some.key")
    static std::string resolve(std::string_view text);

    // Очистить все загруженные локали
    static void clear();

private:
    static inline std::string s_currentLanguage = "ru";
    static inline std::string s_defaultLanguage = "ru";
    // langCode -> (key -> translated string)
    static inline std::unordered_map<std::string, std::unordered_map<std::string, std::string>> s_translations;
};

// Удобный макрос для быстрого доступа к переводам
#define LOC(key) Localization::get(key, key)

#endif //DARIVN_LOCALIZATION_HPP
