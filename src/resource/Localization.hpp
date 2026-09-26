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
