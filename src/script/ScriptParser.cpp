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

#include "ScriptParser.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <optional>

static std::string_view trim(std::string_view s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string unquote(std::string_view s) {
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        return std::string(s.substr(1, s.size() - 2));
    }
    return std::string(s);
}

static size_t findOutsideQuotes(std::string_view s, std::string_view target) {
    bool inDoubleQuote = false;
    bool inSingleQuote = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '"' && !inSingleQuote && (i == 0 || s[i - 1] != '\\')) {
            inDoubleQuote = !inDoubleQuote;
        } else if (c == '\'' && !inDoubleQuote && (i == 0 || s[i - 1] != '\\')) {
            inSingleQuote = !inSingleQuote;
        } else if (!inDoubleQuote && !inSingleQuote) {
            if (i + target.size() <= s.size() && s.substr(i, target.size()) == target) {
                return i;
            }
        }
    }
    return std::string_view::npos;
}

static size_t findOutsideQuotesAndParens(std::string_view s, std::string_view target) {
    bool inDoubleQuote = false;
    bool inSingleQuote = false;
    int parenDepth = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '"' && !inSingleQuote && (i == 0 || s[i - 1] != '\\')) {
            inDoubleQuote = !inDoubleQuote;
        } else if (c == '\'' && !inDoubleQuote && (i == 0 || s[i - 1] != '\\')) {
            inSingleQuote = !inSingleQuote;
        } else if (!inDoubleQuote && !inSingleQuote) {
            if (c == '(') {
                parenDepth++;
            } else if (c == ')') {
                if (parenDepth > 0) parenDepth--;
            } else if (parenDepth == 0) {
                if (i + target.size() <= s.size() && s.substr(i, target.size()) == target) {
                    return i;
                }
            }
        }
    }
    return std::string_view::npos;
}

static std::string stripComments(std::string_view line) {
    bool inDoubleQuote = false;
    bool inSingleQuote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"' && !inSingleQuote && (i == 0 || line[i - 1] != '\\')) {
            inDoubleQuote = !inDoubleQuote;
        } else if (c == '\'' && !inDoubleQuote && (i == 0 || line[i - 1] != '\\')) {
            inSingleQuote = !inSingleQuote;
        } else if (c == '#' && !inDoubleQuote && !inSingleQuote) {
            return std::string(line.substr(0, i));
        }
    }
    return std::string(line);
}

static std::vector<std::string> splitQuotedTokens(std::string_view line) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) i++;
        if (i >= line.size()) break;

        if (line[i] == '"' || line[i] == '\'') {
            char q = line[i];
            size_t start = i + 1;
            size_t end = start;
            while (end < line.size()) {
                if (line[end] == q && (end == 0 || line[end - 1] != '\\')) {
                    break;
                }
                end++;
            }
            tokens.emplace_back(line.substr(start, end - start));
            i = (end < line.size()) ? end + 1 : end;
        } else {
            size_t start = i;
            while (i < line.size() && line[i] != ' ' && line[i] != '\t') i++;
            tokens.emplace_back(line.substr(start, i - start));
        }
    }
    return tokens;
}

static bool isNumber(std::string_view s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (start >= s.size()) return false;
    bool hasDot = false;
    for (size_t i = start; i < s.size(); ++i) {
        if (s[i] == '.') {
            if (hasDot) return false;
            hasDot = true;
        } else if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    }
    return true;
}

static bool parseSetVariable(std::string_view line, ScriptCommand& outCmd) {
    std::string_view s = line;
    if (s.starts_with("set ") || s.starts_with("set\t")) {
        s = trim(s.substr(3));
    }
    if (s.empty()) return false;
    if (!s.starts_with('$') && !line.starts_with("set ") && !line.starts_with("set\t")) {
        return false;
    }

    const std::pair<std::string_view, VariableOp> compoundOps[] = {
        {"+=", VariableOp::AddAssign},
        {"-=", VariableOp::SubAssign},
        {"*=", VariableOp::MulAssign},
        {"/=", VariableOp::DivAssign}
    };

    for (const auto& [opStr, opType] : compoundOps) {
        size_t pos = findOutsideQuotes(s, opStr);
        if (pos != std::string_view::npos) {
            std::string_view varPart = trim(s.substr(0, pos));
            std::string_view valPart = trim(s.substr(pos + opStr.size()));
            if (varPart.starts_with('$')) varPart = varPart.substr(1);
            varPart = trim(varPart);
            if (!varPart.empty()) {
                outCmd = ScriptCommand();
                outCmd.type = CommandType::SetVariable;
                outCmd.varName = std::string(varPart);
                outCmd.varOp = opType;
                outCmd.varExpression = std::string(valPart);
                return true;
            }
        }
    }

    // Проверка оператора '=' (не должен быть '==', '<=', '>=', '!=')
    bool inDQ = false, inSQ = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '"' && !inSQ && (i == 0 || s[i - 1] != '\\')) inDQ = !inDQ;
        else if (c == '\'' && !inDQ && (i == 0 || s[i - 1] != '\\')) inSQ = !inSQ;
        else if (!inDQ && !inSQ && c == '=') {
            char prev = (i > 0) ? s[i - 1] : ' ';
            char next = (i + 1 < s.size()) ? s[i + 1] : ' ';
            if (prev != '=' && prev != '<' && prev != '>' && prev != '!' &&
                prev != '+' && prev != '-' && prev != '*' && prev != '/' &&
                next != '=') {
                std::string_view varPart = trim(s.substr(0, i));
                std::string_view valPart = trim(s.substr(i + 1));
                if (varPart.starts_with('$')) varPart = varPart.substr(1);
                varPart = trim(varPart);
                if (!varPart.empty()) {
                    outCmd = ScriptCommand();
                    outCmd.type = CommandType::SetVariable;
                    outCmd.varName = std::string(varPart);
                    outCmd.varOp = VariableOp::Assign;
                    outCmd.varExpression = std::string(valPart);
                    return true;
                }
            }
        }
    }

    return false;
}

static bool parseIfLine(std::string_view trimmed, std::string& outCondition, std::string& outCmdPart, std::string& outElseLabel, std::string& outElseScript) {
    if (!trimmed.starts_with("if ") && !trimmed.starts_with("if\t")) {
        return false;
    }

    std::string_view rest = trim(trimmed.substr(2));
    if (rest.empty()) return false;

    // Проверяем наличие " else jump "
    size_t elsePos = findOutsideQuotesAndParens(rest, " else jump ");
    std::string_view mainPart = rest;
    if (elsePos != std::string_view::npos) {
        mainPart = trim(rest.substr(0, elsePos));
        std::string elseTarget = std::string(trim(rest.substr(elsePos + 11)));
        auto elseTokens = splitQuotedTokens(elseTarget);
        if (!elseTokens.empty() && elseTokens[0] == "script" && elseTokens.size() >= 2) {
            outElseScript = elseTokens[1];
            if (elseTokens.size() >= 3) outElseLabel = elseTokens[2];
        } else if (!elseTokens.empty() && (elseTokens[0].ends_with(".vn") || elseTokens[0].find('/') != std::string::npos)) {
            outElseScript = elseTokens[0];
            if (elseTokens.size() >= 2) outElseLabel = elseTokens[1];
        } else {
            outElseLabel = elseTarget;
        }
    }

    // Ищем разделитель между условием и телом команды
    size_t splitPos = std::string_view::npos;
    size_t cmdOffset = 0;

    size_t p = findOutsideQuotesAndParens(mainPart, " then ");
    if (p != std::string_view::npos) {
        splitPos = p;
        cmdOffset = 6;
    } else if ((p = findOutsideQuotesAndParens(mainPart, ": ")) != std::string_view::npos) {
        splitPos = p;
        cmdOffset = 2;
    } else if ((p = findOutsideQuotesAndParens(mainPart, " jump ")) != std::string_view::npos) {
        splitPos = p;
        cmdOffset = 1;
    } else {
        const std::string_view kwDelims[] = {
            " scene ", " bg ", " show ", " move ", " hide ",
            " play ", " stop ", " effect ", " shake ", " hop ", " flash ", " set "
        };
        size_t earliest = std::string_view::npos;
        for (auto kw : kwDelims) {
            size_t kp = findOutsideQuotesAndParens(mainPart, kw);
            if (kp != std::string_view::npos && kp < earliest) {
                earliest = kp;
            }
        }
        if (earliest != std::string_view::npos) {
            splitPos = earliest;
            cmdOffset = 1;
        } else if ((p = findOutsideQuotesAndParens(mainPart, " \"")) != std::string_view::npos) {
            splitPos = p;
            cmdOffset = 1;
        } else {
            // Проверяем операцию присваивания переменной ($var = val, $var += val)
            size_t eqPos = findOutsideQuotesAndParens(mainPart, " = ");
            if (eqPos == std::string_view::npos) eqPos = findOutsideQuotesAndParens(mainPart, " += ");
            if (eqPos == std::string_view::npos) eqPos = findOutsideQuotesAndParens(mainPart, " -= ");
            if (eqPos == std::string_view::npos) eqPos = findOutsideQuotesAndParens(mainPart, " *= ");
            if (eqPos == std::string_view::npos) eqPos = findOutsideQuotesAndParens(mainPart, " /= ");

            if (eqPos != std::string_view::npos) {
                size_t varStart = mainPart.rfind('$', eqPos);
                if (varStart != std::string_view::npos && varStart > 0) {
                    splitPos = varStart;
                    cmdOffset = 0;
                }
            }
        }
    }

    if (splitPos != std::string_view::npos) {
        outCondition = std::string(trim(mainPart.substr(0, splitPos)));
        outCmdPart = std::string(trim(mainPart.substr(splitPos + cmdOffset)));
        return true;
    }

    return false;
}

static bool parseSingleCommand(std::string_view trimmed,
                               const std::string& condition,
                               const std::string& elseLabel,
                               const std::string& elseScript,
                               ScriptCommand& cmd) {
    // 0. Присваивание/модификация переменной
    if (parseSetVariable(trimmed, cmd)) {
        cmd.condition = condition;
        return true;
    }

    auto tokens = splitQuotedTokens(trimmed);
    if (tokens.empty()) return false;

    // 1. Определение персонажа в сценарии:
    if (tokens[0] == "define" && tokens.size() >= 4) {
        if (tokens[2] == "expression" && tokens.size() >= 5) {
            cmd = ScriptCommand();
            cmd.type = CommandType::DefineExpression;
            cmd.characterName = tokens[1];
            cmd.expression = tokens[3];
            cmd.path = tokens[4];
            cmd.condition = condition;
            return true;
        } else {
            cmd = ScriptCommand();
            cmd.type = CommandType::DefineCharacter;
            cmd.characterName = tokens[1];
            cmd.displayName = tokens[2];
            cmd.path = tokens[3];
            for (size_t i = 4; i < tokens.size(); ++i) {
                if (tokens[i] == "scale" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                    cmd.scale = std::stof(tokens[i + 1]);
                    cmd.hasCustomScale = true;
                    i++;
                }
            }
            cmd.condition = condition;
            return true;
        }
    }

    // 2. Смена фона: scene <path> [with <fade|dissolve|crossfade> [duration]]
    if (tokens[0] == "scene" || tokens[0] == "bg") {
        if (tokens.size() >= 2) {
            cmd = ScriptCommand();
            cmd.type = CommandType::SetBackground;
            cmd.path = tokens[1];
            for (size_t i = 2; i < tokens.size(); ++i) {
                if (tokens[i] == "with" && i + 1 < tokens.size()) {
                    cmd.transitionType = tokens[i + 1];
                    if (i + 2 < tokens.size() && isNumber(tokens[i + 2])) {
                        cmd.transitionDuration = std::stof(tokens[i + 2]);
                        i += 2;
                    } else {
                        cmd.transitionDuration = 1.0f;
                        i += 1;
                    }
                }
            }
            cmd.condition = condition;
            return true;
        }
    }

    // 2.1 Автономный переход сцены: transition <fade|dissolve|crossfade> [duration]
    if (tokens[0] == "transition" && tokens.size() >= 2) {
        cmd = ScriptCommand();
        cmd.type = CommandType::Transition;
        cmd.transitionType = tokens[1];
        cmd.transitionDuration = (tokens.size() >= 3 && isNumber(tokens[2])) ? std::stof(tokens[2]) : 1.0f;
        cmd.condition = condition;
        return true;
    }

    // 2.2 Загрузка конфигурации персонажа из отдельного файла: load character <id> [path]
    if ((tokens[0] == "load" || tokens[0] == "import") && tokens.size() >= 3 && tokens[1] == "character") {
        cmd = ScriptCommand();
        cmd.type = CommandType::LoadCharacter;
        cmd.characterName = tokens[2];
        if (tokens.size() >= 4) {
            cmd.path = tokens[3];
        }
        cmd.condition = condition;
        return true;
    }

    // 2.3 Управление музыкой (BGM):
    if (tokens[0] == "play" && tokens.size() >= 3 && tokens[1] == "music") {
        cmd = ScriptCommand();
        cmd.type = CommandType::PlayMusic;
        cmd.path = tokens[2];
        cmd.audioLoop = true;
        cmd.audioFade = 0.0f;
        cmd.audioVolume = 1.0f;
        for (size_t i = 3; i < tokens.size(); ++i) {
            if (tokens[i] == "noloop") cmd.audioLoop = false;
            else if (tokens[i] == "loop") cmd.audioLoop = true;
            else if (tokens[i] == "fade" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                cmd.audioFade = std::stof(tokens[++i]);
            } else if (tokens[i] == "volume" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                cmd.audioVolume = std::stof(tokens[++i]);
            }
        }
        cmd.condition = condition;
        return true;
    }
    if (tokens[0] == "stop" && tokens.size() >= 2 && tokens[1] == "music") {
        cmd = ScriptCommand();
        cmd.type = CommandType::StopMusic;
        cmd.audioFade = 0.0f;
        for (size_t i = 2; i < tokens.size(); ++i) {
            if (tokens[i] == "fade" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                cmd.audioFade = std::stof(tokens[++i]);
            }
        }
        cmd.condition = condition;
        return true;
    }
    if (tokens[0] == "music" && tokens.size() >= 2) {
        if (tokens[1] == "stop") {
            cmd = ScriptCommand();
            cmd.type = CommandType::StopMusic;
            cmd.audioFade = 0.0f;
            if (tokens.size() >= 3 && isNumber(tokens[2])) cmd.audioFade = std::stof(tokens[2]);
            cmd.condition = condition;
            return true;
        } else {
            size_t pIdx = (tokens[1] == "play" && tokens.size() >= 3) ? 2 : 1;
            cmd = ScriptCommand();
            cmd.type = CommandType::PlayMusic;
            cmd.path = tokens[pIdx];
            cmd.audioLoop = true;
            cmd.audioFade = 0.0f;
            cmd.audioVolume = 1.0f;
            for (size_t i = pIdx + 1; i < tokens.size(); ++i) {
                if (tokens[i] == "noloop") cmd.audioLoop = false;
                else if (tokens[i] == "loop") cmd.audioLoop = true;
                else if (tokens[i] == "fade" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                    cmd.audioFade = std::stof(tokens[++i]);
                } else if (tokens[i] == "volume" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                    cmd.audioVolume = std::stof(tokens[++i]);
                }
            }
            cmd.condition = condition;
            return true;
        }
    }

    // 2.4 Звуковые эффекты (SFX):
    if ((tokens[0] == "play" && tokens.size() >= 3 && (tokens[1] == "sound" || tokens[1] == "sfx")) ||
        ((tokens[0] == "sound" || tokens[0] == "sfx") && tokens.size() >= 2)) {
        size_t pIdx = (tokens[0] == "play") ? 2 : 1;
        cmd = ScriptCommand();
        cmd.type = CommandType::PlaySound;
        cmd.path = tokens[pIdx];
        cmd.audioVolume = 1.0f;
        for (size_t i = pIdx + 1; i < tokens.size(); ++i) {
            if (tokens[i] == "volume" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                cmd.audioVolume = std::stof(tokens[++i]);
            }
        }
        cmd.condition = condition;
        return true;
    }

    // 2.5 Озвучка (Voice):
    if (tokens[0] == "stop" && tokens.size() >= 2 && tokens[1] == "voice") {
        cmd = ScriptCommand();
        cmd.type = CommandType::PlayVoice;
        cmd.path = "";
        cmd.condition = condition;
        return true;
    }
    if ((tokens[0] == "play" && tokens.size() >= 3 && tokens[1] == "voice") ||
        (tokens[0] == "voice" && tokens.size() >= 2)) {
        size_t pIdx = (tokens[0] == "play") ? 2 : 1;
        cmd = ScriptCommand();
        cmd.type = CommandType::PlayVoice;
        cmd.path = tokens[pIdx];
        cmd.audioVolume = 1.0f;
        for (size_t i = pIdx + 1; i < tokens.size(); ++i) {
            if (tokens[i] == "volume" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                cmd.audioVolume = std::stof(tokens[++i]);
            }
        }
        cmd.condition = condition;
        return true;
    }

    // 2.6 Фоновое окружение / Эмбиент (Ambient / BGS):
    if (tokens[0] == "stop" && tokens.size() >= 2 && (tokens[1] == "ambient" || tokens[1] == "bgs")) {
        cmd = ScriptCommand();
        cmd.type = CommandType::StopAmbient;
        cmd.audioFade = 0.0f;
        for (size_t i = 2; i < tokens.size(); ++i) {
            if (tokens[i] == "fade" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                cmd.audioFade = std::stof(tokens[++i]);
            }
        }
        cmd.condition = condition;
        return true;
    }
    if ((tokens[0] == "play" && tokens.size() >= 3 && (tokens[1] == "ambient" || tokens[1] == "bgs")) ||
        ((tokens[0] == "ambient" || tokens[0] == "bgs") && tokens.size() >= 2)) {
        size_t pIdx = (tokens[0] == "play") ? 2 : 1;
        cmd = ScriptCommand();
        cmd.type = CommandType::PlayAmbient;
        cmd.path = tokens[pIdx];
        cmd.audioLoop = true;
        cmd.audioFade = 0.0f;
        cmd.audioVolume = 1.0f;
        for (size_t i = pIdx + 1; i < tokens.size(); ++i) {
            if (tokens[i] == "noloop") cmd.audioLoop = false;
            else if (tokens[i] == "loop") cmd.audioLoop = true;
            else if (tokens[i] == "fade" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                cmd.audioFade = std::stof(tokens[++i]);
            } else if (tokens[i] == "volume" && i + 1 < tokens.size() && isNumber(tokens[i + 1])) {
                cmd.audioVolume = std::stof(tokens[++i]);
            }
        }
        cmd.condition = condition;
        return true;
    }

    // 3. Отобразить или переместить персонажа:
    if ((tokens[0] == "show" || tokens[0] == "move") && tokens.size() >= 2) {
        cmd = ScriptCommand();
        cmd.type = CommandType::ShowCharacter;
        cmd.characterName = tokens[1];
        cmd.expression = "";
        cmd.positionSlot = "";
        cmd.layer = 0;
        cmd.hasCustomLayer = false;
        cmd.hasCustomX = false;
        cmd.hasCustomY = false;
        cmd.hasCustomCoords = false;
        cmd.scale = 1.0f;
        cmd.hasCustomScale = false;

        size_t idx = 2;
        if (idx < tokens.size() && tokens[idx] != "at" && tokens[idx] != "to" && tokens[idx] != "layer" && tokens[idx] != "scale") {
            if (!isNumber(tokens[idx]) && tokens[idx] != "left" && tokens[idx] != "center" && tokens[idx] != "right") {
                cmd.expression = tokens[idx];
                idx++;
            }
        }

        if (idx < tokens.size() && (tokens[idx] == "at" || tokens[idx] == "to")) {
            idx++;
        }

        if (idx < tokens.size()) {
            if (isNumber(tokens[idx])) {
                cmd.hasCustomX = true;
                cmd.posX = std::stof(tokens[idx]);
                idx++;

                if (idx < tokens.size() && isNumber(tokens[idx])) {
                    cmd.hasCustomY = true;
                    cmd.hasCustomCoords = true;
                    cmd.posY = std::stof(tokens[idx]);
                    idx++;
                }
            } else if (tokens[idx] == "left" || tokens[idx] == "center" || tokens[idx] == "right") {
                cmd.positionSlot = tokens[idx];
                idx++;
            }
        }

        while (idx < tokens.size()) {
            if (tokens[idx] == "layer" && idx + 1 < tokens.size() && isNumber(tokens[idx + 1])) {
                cmd.layer = std::stoi(tokens[idx + 1]);
                cmd.hasCustomLayer = true;
                idx += 2;
            } else if (tokens[idx] == "scale" && idx + 1 < tokens.size() && isNumber(tokens[idx + 1])) {
                cmd.scale = std::stof(tokens[idx + 1]);
                cmd.hasCustomScale = true;
                idx += 2;
            } else {
                idx++;
            }
        }

        cmd.condition = condition;
        return true;
    }

    // 4. Скрыть персонажа: hide <id>
    if (tokens[0] == "hide" && tokens.size() >= 2) {
        cmd = ScriptCommand();
        cmd.type = CommandType::HideCharacter;
        cmd.characterName = tokens[1];
        cmd.condition = condition;
        return true;
    }

    // 5. Метка перехода: label <name>
    if (tokens[0] == "label" && tokens.size() >= 2) {
        cmd = ScriptCommand();
        cmd.type = CommandType::Label;
        cmd.targetLabel = tokens[1];
        cmd.condition = condition;
        return true;
    }

    // 6. Переход: jump <name> или jump script <path> [label]
    if (tokens[0] == "jump" && tokens.size() >= 2) {
        cmd = ScriptCommand();
        if (tokens[1] == "script" && tokens.size() >= 3) {
            cmd.type = CommandType::JumpScript;
            cmd.targetScript = tokens[2];
            if (tokens.size() >= 4) cmd.targetLabel = tokens[3];
        } else if (tokens[1].ends_with(".vn") || tokens[1].find('/') != std::string::npos) {
            cmd.type = CommandType::JumpScript;
            cmd.targetScript = tokens[1];
            if (tokens.size() >= 3) cmd.targetLabel = tokens[2];
        } else {
            cmd.type = CommandType::Jump;
            cmd.targetLabel = tokens[1];
        }
        cmd.condition = condition;
        cmd.elseLabel = elseLabel;
        cmd.elseScript = elseScript;
        return true;
    }

    // 7. Пост-эффекты: экранные и персонажные
    if (tokens[0] == "effect" && tokens.size() >= 2) {
        bool isScreenEffect = (tokens[1] == "vignette" || tokens[1] == "blur") ||
                              ((tokens[1] == "sepia" || tokens[1] == "tint" || tokens[1] == "clear" || tokens[1] == "reset") &&
                               (tokens.size() == 2 || isNumber(tokens[2])));

        if (isScreenEffect) {
            cmd = ScriptCommand();
            cmd.type = CommandType::Effect;
            cmd.effectType = tokens[1];
            if (tokens.size() >= 3 && isNumber(tokens[2])) cmd.effectValue1 = std::stof(tokens[2]);
            if (tokens.size() >= 4 && isNumber(tokens[3])) cmd.effectValue2 = std::stof(tokens[3]);
            if (tokens.size() >= 5 && isNumber(tokens[4])) cmd.effectValue3 = std::stof(tokens[4]);
            if (tokens.size() >= 6 && isNumber(tokens[5])) cmd.effectValue4 = std::stof(tokens[5]);
            cmd.condition = condition;
            return true;
        } else if (tokens.size() >= 3) {
            cmd = ScriptCommand();
            cmd.type = CommandType::CharacterEffect;
            cmd.characterName = tokens[1];
            cmd.effectType = tokens[2];
            if (cmd.effectType == "silhouette") {
                cmd.effectValue1 = (tokens.size() >= 4 && (tokens[3] == "off" || tokens[3] == "false" || tokens[3] == "0")) ? 0.0f : 1.0f;
            } else {
                if (tokens.size() >= 4 && isNumber(tokens[3])) cmd.effectValue1 = std::stof(tokens[3]);
                if (tokens.size() >= 5 && isNumber(tokens[4])) cmd.effectValue2 = std::stof(tokens[4]);
                if (tokens.size() >= 6 && isNumber(tokens[5])) cmd.effectValue3 = std::stof(tokens[5]);
                if (tokens.size() >= 7 && isNumber(tokens[6])) cmd.effectValue4 = std::stof(tokens[6]);
            }
            cmd.condition = condition;
            return true;
        }
    }

    // 8. Дрожь/тряска персонажа: shake <id> [duration=0.4] [intensity=15.0]
    if (tokens[0] == "shake" && tokens.size() >= 2) {
        cmd = ScriptCommand();
        cmd.type = CommandType::CharacterShake;
        cmd.characterName = tokens[1];
        cmd.effectValue1 = (tokens.size() >= 3 && isNumber(tokens[2])) ? std::stof(tokens[2]) : 0.4f;
        cmd.effectValue2 = (tokens.size() >= 4 && isNumber(tokens[3])) ? std::stof(tokens[3]) : 15.0f;
        cmd.condition = condition;
        return true;
    }

    // 9. Подскок персонажа: hop <id> [duration=0.3] [height=25.0]
    if (tokens[0] == "hop" && tokens.size() >= 2) {
        cmd = ScriptCommand();
        cmd.type = CommandType::CharacterHop;
        cmd.characterName = tokens[1];
        cmd.effectValue1 = (tokens.size() >= 3 && isNumber(tokens[2])) ? std::stof(tokens[2]) : 0.3f;
        cmd.effectValue2 = (tokens.size() >= 4 && isNumber(tokens[3])) ? std::stof(tokens[3]) : 25.0f;
        cmd.condition = condition;
        return true;
    }

    // 10. Вспышка персонажа: flash <id> [duration=0.25]
    if (tokens[0] == "flash" && tokens.size() >= 2) {
        cmd = ScriptCommand();
        cmd.type = CommandType::CharacterFlash;
        cmd.characterName = tokens[1];
        cmd.effectValue1 = (tokens.size() >= 3 && isNumber(tokens[2])) ? std::stof(tokens[2]) : 0.25f;
        cmd.condition = condition;
        return true;
    }

    // 11. Реплика диалога с ключом локализации без кавычек: speaker @key или @key
    if (tokens.size() == 1 && tokens[0].starts_with("@")) {
        cmd = ScriptCommand();
        cmd.type = CommandType::Say;
        cmd.speaker = "";
        cmd.text = tokens[0];
        cmd.condition = condition;
        return true;
    }
    if (tokens.size() == 2 && tokens[1].starts_with("@")) {
        cmd = ScriptCommand();
        cmd.type = CommandType::Say;
        cmd.speaker = tokens[0];
        cmd.text = tokens[1];
        cmd.condition = condition;
        return true;
    }

    // 12. Реплика диалога: speaker "Текст" или "Текст"
    size_t firstQuote = trimmed.find('"');
    size_t lastQuote = trimmed.rfind('"');

    if (firstQuote != std::string_view::npos && lastQuote != std::string_view::npos && firstQuote < lastQuote) {
        cmd = ScriptCommand();
        cmd.type = CommandType::Say;
        cmd.text = trimmed.substr(firstQuote + 1, lastQuote - firstQuote - 1);

        if (firstQuote > 0) {
            cmd.speaker = trim(trimmed.substr(0, firstQuote));
        } else {
            cmd.speaker = "";
        }

        cmd.condition = condition;
        return true;
    }

    return false;
}

std::vector<ScriptCommand> ScriptParser::parseFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ScriptParser] Failed to open script file: " << filePath << std::endl;
        return {};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str());
}

std::vector<ScriptCommand> ScriptParser::parseString(const std::string& content) {
    std::vector<ScriptCommand> commands;
    std::istringstream stream(content);
    std::string line;

    bool inChoiceBlock = false;
    ScriptCommand currentChoiceCmd;

    while (std::getline(stream, line)) {
        std::string stripped = stripComments(line);
        std::string_view trimmed = trim(stripped);
        if (trimmed.empty()) {
            continue;
        }

        // Если мы внутри блока choice
        if (inChoiceBlock) {
            size_t arrowPos = findOutsideQuotes(trimmed, "->");
            if (arrowPos != std::string_view::npos && (line.starts_with(' ') || line.starts_with('\t'))) {
                std::string_view optionPart = trim(trimmed.substr(0, arrowPos));
                std::string_view targetPart = trim(trimmed.substr(arrowPos + 2));
                auto targetTokens = splitQuotedTokens(targetPart);

                ChoiceOption opt;
                size_t ifPos = findOutsideQuotes(optionPart, " if ");
                std::string_view rawOptionText = optionPart;
                if (ifPos != std::string_view::npos) {
                    rawOptionText = trim(optionPart.substr(0, ifPos));
                    opt.condition = std::string(trim(optionPart.substr(ifPos + 4)));
                } else {
                    size_t lastQ = optionPart.rfind('"');
                    if (lastQ == std::string_view::npos) lastQ = optionPart.rfind('\'');
                    if (lastQ != std::string_view::npos && lastQ + 1 < optionPart.size()) {
                        std::string_view afterQuote = trim(optionPart.substr(lastQ + 1));
                        if (afterQuote.starts_with("if ") || afterQuote.starts_with("if\t")) {
                            rawOptionText = trim(optionPart.substr(0, lastQ + 1));
                            opt.condition = std::string(trim(afterQuote.substr(2)));
                        }
                    }
                }

                opt.text = unquote(rawOptionText);
                if (!targetTokens.empty() && targetTokens[0] == "script" && targetTokens.size() >= 2) {
                    opt.targetScript = targetTokens[1];
                    if (targetTokens.size() >= 3) opt.targetLabel = targetTokens[2];
                } else if (!targetTokens.empty() && (targetTokens[0].ends_with(".vn") || targetTokens[0].find('/') != std::string::npos)) {
                    opt.targetScript = targetTokens[0];
                    if (targetTokens.size() >= 2) opt.targetLabel = targetTokens[1];
                } else {
                    opt.targetLabel = targetPart;
                }
                currentChoiceCmd.choices.push_back(opt);
                continue;
            } else {
                commands.push_back(currentChoiceCmd);
                currentChoiceCmd = ScriptCommand();
                inChoiceBlock = false;
            }
        }

        // Начало блока choice:
        if (trimmed == "choice:" || trimmed.starts_with("choice")) {
            inChoiceBlock = true;
            currentChoiceCmd.type = CommandType::Choice;
            continue;
        }

        // Проверяем, является ли строка условной командой: if <condition> <command>
        std::string condition, cmdPart, elseLabel, elseScript;
        if (parseIfLine(trimmed, condition, cmdPart, elseLabel, elseScript)) {
            ScriptCommand cmd;
            if (parseSingleCommand(cmdPart, condition, elseLabel, elseScript, cmd)) {
                commands.push_back(cmd);
            } else {
                std::cerr << "[ScriptParser] Failed to parse conditional command: " << cmdPart << std::endl;
            }
            continue;
        }

        // Обычная команда
        ScriptCommand cmd;
        if (parseSingleCommand(trimmed, "", "", "", cmd)) {
            commands.push_back(cmd);
        }
    }

    if (inChoiceBlock) {
        commands.push_back(currentChoiceCmd);
    }

    std::cout << "[ScriptParser] Parsed " << commands.size() << " commands from script" << std::endl;
    return commands;
}
