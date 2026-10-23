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

#ifndef DARIVN_GLOSSARYMANAGER_HPP
#define DARIVN_GLOSSARYMANAGER_HPP

#include <string>
#include <string_view>
#include <unordered_map>

struct GlossaryTerm {
    std::string id;
    std::string title;
    std::string description;
};

class GlossaryManager {
public:
    GlossaryManager() = delete;

    // Загрузка словаря терминов из файла (.vn / .ini)
    static bool loadFromFile(const std::string& filePath);

    // Добавление / обновление отдельного термина
    static void addTerm(const std::string& id, const std::string& title, const std::string& description);

    // Получение информации о термине по его ID
    static const GlossaryTerm* getTerm(std::string_view id);

    // Проверка наличия термина в словаре
    static bool hasTerm(std::string_view id);

    // Все загруженные термины
    static const std::unordered_map<std::string, GlossaryTerm>& getAllTerms();

    // Очистка словаря
    static void clear();

private:
    static inline std::unordered_map<std::string, GlossaryTerm> s_terms;
};

#endif // DARIVN_GLOSSARYMANAGER_HPP
