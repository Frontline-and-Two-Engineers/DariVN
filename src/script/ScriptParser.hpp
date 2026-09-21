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
