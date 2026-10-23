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

#include "GlossaryManager.hpp"
#include "vfs/VFS.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

static std::string trimString(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    std::string s = str.substr(first, (last - first + 1));
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

bool GlossaryManager::loadFromFile(const std::string& filePath) {
    std::string content;
    auto vfsData = DariVN::VFS::readFile(filePath);
    if (!vfsData.empty()) {
        content = std::string(reinterpret_cast<const char*>(vfsData.data()), vfsData.size());
    } else {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        content = ss.str();
    }

    std::stringstream stream(content);
    std::string line;
    std::string currentSection;
    std::string currentTitle;
    std::string currentDesc;

    auto flushSection = [&]() {
        if (!currentSection.empty()) {
            if (currentTitle.empty()) {
                currentTitle = currentSection;
                if (!currentTitle.empty()) currentTitle[0] = static_cast<char>(std::toupper(currentTitle[0]));
            }
            s_terms[currentSection] = GlossaryTerm{currentSection, currentTitle, currentDesc};
        }
        currentSection.clear();
        currentTitle.clear();
        currentDesc.clear();
    };

    while (std::getline(stream, line)) {
        std::string trimmed = trimString(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed.starts_with("//")) {
            continue;
        }

        // Секция в квадратных скобках: [term_id]
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            flushSection();
            currentSection = trimString(trimmed.substr(1, trimmed.size() - 2));
            continue;
        }

        size_t delimiterPos = trimmed.find('=');
        if (delimiterPos == std::string::npos) {
            delimiterPos = trimmed.find(':');
        }

        if (delimiterPos != std::string::npos) {
            std::string key = trimString(trimmed.substr(0, delimiterPos));
            std::string val = trimString(trimmed.substr(delimiterPos + 1));

            if (!currentSection.empty()) {
                std::string lowerKey = key;
                std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), [](unsigned char c) { return std::tolower(c); });
                if (lowerKey == "title" || lowerKey == "name") {
                    currentTitle = val;
                } else if (lowerKey == "text" || lowerKey == "desc" || lowerKey == "description") {
                    currentDesc = val;
                }
            } else {
                // Прямой формат: term_id = Заголовок | Пояснение
                // или: term_id = Пояснение
                size_t pipePos = val.find('|');
                std::string title;
                std::string desc;
                if (pipePos != std::string::npos) {
                    title = trimString(val.substr(0, pipePos));
                    desc = trimString(val.substr(pipePos + 1));
                } else {
                    title = key;
                    if (!title.empty()) title[0] = static_cast<char>(std::toupper(title[0]));
                    desc = val;
                }
                s_terms[key] = GlossaryTerm{key, title, desc};
            }
        }
    }
    flushSection();

    std::cout << "[GlossaryManager] Loaded glossary from " << filePath 
              << " (" << s_terms.size() << " terms)" << std::endl;
    return true;
}

void GlossaryManager::addTerm(const std::string& id, const std::string& title, const std::string& description) {
    s_terms[id] = GlossaryTerm{id, title, description};
}

const GlossaryTerm* GlossaryManager::getTerm(std::string_view id) {
    auto it = s_terms.find(std::string(id));
    if (it != s_terms.end()) {
        return &it->second;
    }
    return nullptr;
}

bool GlossaryManager::hasTerm(std::string_view id) {
    return s_terms.find(std::string(id)) != s_terms.end();
}

const std::unordered_map<std::string, GlossaryTerm>& GlossaryManager::getAllTerms() {
    return s_terms;
}

void GlossaryManager::clear() {
    s_terms.clear();
}
