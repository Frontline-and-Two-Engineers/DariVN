#include "Localization.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

static std::string trim(std::string_view s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return std::string(s.substr(start, end - start + 1));
}

static std::string unquote(std::string_view s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return std::string(s.substr(1, s.size() - 2));
    }
    return std::string(s);
}

// Заменяет символы "\n" на реальный символ перевода строки
static std::string unescapeNewlines(std::string str) {
    size_t pos = 0;
    while ((pos = str.find("\\n", pos)) != std::string::npos) {
        str.replace(pos, 2, "\n");
        pos += 1;
    }
    return str;
}

bool Localization::loadLocale(const std::string& langCode, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[Localization] Failed to open locale file: " << filePath << std::endl;
        return false;
    }

    auto& dict = s_translations[langCode];
    std::string line;
    int count = 0;

    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.starts_with('#') || trimmed.starts_with(';')) {
            continue;
        }

        size_t equalPos = trimmed.find('=');
        if (equalPos == std::string::npos) {
            continue;
        }

        std::string key = trim(trimmed.substr(0, equalPos));
        std::string val = trim(trimmed.substr(equalPos + 1));

        val = unquote(val);
        val = unescapeNewlines(val);

        if (!key.empty()) {
            dict[key] = val;
            count++;
        }
    }

    std::cout << "[Localization] Loaded " << count << " strings for locale '" 
              << langCode << "' from " << filePath << std::endl;
    return true;
}

bool Localization::setLanguage(const std::string& langCode) {
    auto it = s_translations.find(langCode);
    if (it == s_translations.end()) {
        std::cerr << "[Localization] Language '" << langCode << "' is not loaded!" << std::endl;
        return false;
    }

    s_currentLanguage = langCode;
    std::cout << "[Localization] Switched active language to: " << langCode << std::endl;
    return true;
}

void Localization::setDefaultLanguage(const std::string& langCode) {
    s_defaultLanguage = langCode;
}

const std::string& Localization::getDefaultLanguage() {
    return s_defaultLanguage;
}

const std::string& Localization::getCurrentLanguage() {
    return s_currentLanguage;
}

std::vector<std::string> Localization::getAvailableLanguages() {
    std::vector<std::string> langs;
    langs.reserve(s_translations.size());
    for (const auto& [code, _] : s_translations) {
        langs.push_back(code);
    }
    return langs;
}

bool Localization::has(std::string_view key) {
    std::string k(key);
    auto langIt = s_translations.find(s_currentLanguage);
    if (langIt != s_translations.end() && langIt->second.contains(k)) {
        return true;
    }
    if (s_currentLanguage != s_defaultLanguage) {
        auto defIt = s_translations.find(s_defaultLanguage);
        if (defIt != s_translations.end() && defIt->second.contains(k)) {
            return true;
        }
    }
    return false;
}

std::string Localization::get(std::string_view key, std::string_view defaultValue) {
    std::string k(key);

    // 1. Ищем в текущем активном языке
    auto langIt = s_translations.find(s_currentLanguage);
    if (langIt != s_translations.end()) {
        auto valIt = langIt->second.find(k);
        if (valIt != langIt->second.end()) {
            return valIt->second;
        }
    }

    // 2. Фолбэк на язык по умолчанию, если текущий язык другой
    if (s_currentLanguage != s_defaultLanguage) {
        auto defIt = s_translations.find(s_defaultLanguage);
        if (defIt != s_translations.end()) {
            auto valIt = defIt->second.find(k);
            if (valIt != defIt->second.end()) {
                return valIt->second;
            }
        }
    }

    // 3. Если перевод не найден — возвращаем defaultValue или сам ключ
    if (!defaultValue.empty()) {
        return std::string(defaultValue);
    }
    return k;
}

std::string Localization::resolve(std::string_view text) {
    if (text.empty()) return "";

    // 1. Если вся строка начинается с '@' (например, "@story.intro" или "@choice.opt1"):
    if (text.starts_with('@')) {
        std::string_view key = text.substr(1);
        // Если это одиночный токен без пробелов (или в кавычках):
        if (key.find(' ') == std::string_view::npos) {
            if (has(key)) {
                return resolve(get(key));
            }
            return std::string(text);
        }
    }

    // 2. Инлайн-подстановки {loc=key} и @{key} внутри текста
    std::string result(text);

    // Подстановка {loc=...}
    size_t pos = 0;
    while ((pos = result.find("{loc=", pos)) != std::string::npos) {
        size_t close = result.find('}', pos + 5);
        if (close == std::string::npos) break;
        std::string key = result.substr(pos + 5, close - (pos + 5));
        std::string val = get(key);
        result.replace(pos, close - pos + 1, val);
        pos += val.size();
    }

    // Подстановка @{...}
    pos = 0;
    while ((pos = result.find("@{", pos)) != std::string::npos) {
        size_t close = result.find('}', pos + 2);
        if (close == std::string::npos) break;
        std::string key = result.substr(pos + 2, close - (pos + 2));
        std::string val = get(key);
        result.replace(pos, close - pos + 1, val);
        pos += val.size();
    }

    return result;
}

void Localization::clear() {
    s_translations.clear();
    s_currentLanguage = "ru";
    s_defaultLanguage = "ru";
}
