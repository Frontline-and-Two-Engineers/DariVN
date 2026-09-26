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

#ifndef DARIVN_SCRIPTPARSER_HPP
#define DARIVN_SCRIPTPARSER_HPP

#include "ScriptCommand.hpp"
#include <string>
#include <vector>

class ScriptParser {
public:
    ScriptParser() = delete;

    static std::vector<ScriptCommand> parseFile(const std::string& filePath);
    static std::vector<ScriptCommand> parseString(const std::string& content);
};

#endif //DARIVN_SCRIPTPARSER_HPP
