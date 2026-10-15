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

#include "StoryEngine.hpp"
#include "ScriptParser.hpp"
#include "resource/Localization.hpp"
#include "resource/ResourceManager.hpp"
#include "renderer/Shader.hpp"
#include "core/SaveManager.hpp"
#include "vfs/VFS.hpp"
#include <iostream>

#include <fstream>
#include <sstream>
#include <filesystem>

StoryEngine::StoryEngine() = default;

bool StoryEngine::loadCharacterConfig(const std::string& charId, const std::string& specificPath) {
    std::string path = specificPath;
    if (path.empty()) {
        std::vector<std::string> candidates = {
            "assets/characters/" + charId + ".vn",
            "assets/characters/" + charId + ".ini",
            "characters/" + charId + ".vn",
            "characters/" + charId + ".ini"
        };
        for (const auto& c : candidates) {
            std::string resolved = m_assetResolver ? m_assetResolver(c) : c;
            if (DariVN::VFS::exists(resolved)) {
                path = resolved;
                break;
            }
        }
    } else {
        path = m_assetResolver ? m_assetResolver(path) : path;
    }

    std::string content = DariVN::VFS::readTextFile(path);
    if (content.empty()) {
        if (path.empty() || !DariVN::VFS::exists(path)) {
            return false;
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        content = ss.str();
    }

    std::istringstream file(content);

    auto& def = m_definedCharacters[charId];
    def.id = charId;

    if (def.displayName.empty()) def.displayName = charId;

    auto trimStr = [](std::string s) {
        size_t s1 = s.find_first_not_of(" \t\r\n");
        if (s1 == std::string::npos) return std::string();
        size_t s2 = s.find_last_not_of(" \t\r\n");
        s = s.substr(s1, s2 - s1 + 1);
        if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
            return s.substr(1, s.size() - 2);
        }
        return s;
    };

    std::string line;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(start, end - start + 1);

        if (trimmed.empty() || trimmed[0] == '#' || trimmed.starts_with("//")) continue;

        size_t eq = trimmed.find('=');
        if (eq == std::string::npos) eq = trimmed.find(':');
        if (eq != std::string::npos) {
            std::string key = trimStr(trimmed.substr(0, eq));
            std::string val = trimStr(trimmed.substr(eq + 1));

            if (key == "id") {
                def.id = val;
            } else if (key == "name" || key == "displayName" || key == "display_name") {
                def.displayName = val;
            } else if (key == "scale") {
                try {
                    def.scale = std::stof(val);
                    def.hasCustomScale = true;
                } catch (...) {}
            } else if (key == "silhouette") {
                def.defaultSilhouette = (val == "true" || val == "1" || val == "yes" || val == "on");
            } else if (key.starts_with("expr.")) {
                std::string exprName = key.substr(5);
                def.expressions[exprName] = val;
            } else if (key == "normal" || key == "happy" || key == "sad" || key == "angry" || key == "shock") {
                def.expressions[key] = val;
            } else if (key == "shader" || key == "defaultShader" || key == "default_shader") {
                def.defaultShader = val;
            }
        }
    }

    std::cout << "[StoryEngine] Loaded character config '" << charId << "' (" << def.displayName << ") from " << path << std::endl;
    return true;
}

bool StoryEngine::loadShaderConfig(const std::string& shaderId, const std::string& specificPath) {
    std::string path = specificPath;
    if (path.empty()) {
        std::vector<std::string> candidates = {
            "assets/shaders/" + shaderId + ".vn",
            "assets/shaders/" + shaderId + ".ini",
            "shaders/" + shaderId + ".vn",
            "shaders/" + shaderId + ".ini"
        };
        for (const auto& c : candidates) {
            std::string resolved = m_assetResolver ? m_assetResolver(c) : c;
            if (DariVN::VFS::exists(resolved)) {
                path = resolved;
                break;
            }
        }
    } else {
        path = m_assetResolver ? m_assetResolver(path) : path;
    }

    std::string content = DariVN::VFS::readTextFile(path);
    if (content.empty()) {
        if (path.empty() || !DariVN::VFS::exists(path)) {
            std::cerr << "[StoryEngine] Warning: Shader config file not found for: " << shaderId << std::endl;
            return false;
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[StoryEngine] Failed to open shader config file: " << path << std::endl;
            return false;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        content = ss.str();
    }

    std::istringstream file(content);


    auto trimStr = [](std::string s) {
        size_t s1 = s.find_first_not_of(" \t\r\n");
        if (s1 == std::string::npos) return std::string();
        size_t s2 = s.find_last_not_of(" \t\r\n");
        s = s.substr(s1, s2 - s1 + 1);
        if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
            return s.substr(1, s.size() - 2);
        }
        return s;
    };

    auto& def = m_definedShaders[shaderId];
    def.id = shaderId;
    def.vertexPath = "assets/shaders/sprite.vert";
    def.fragmentPath.clear();
    def.defaultUniforms.clear();

    std::string line;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(start, end - start + 1);

        if (trimmed.empty() || trimmed[0] == '#' || trimmed.starts_with("//")) continue;

        size_t eq = trimmed.find('=');
        if (eq == std::string::npos) eq = trimmed.find(':');
        if (eq != std::string::npos) {
            std::string key = trimStr(trimmed.substr(0, eq));
            std::string val = trimStr(trimmed.substr(eq + 1));

            if (key == "id") {
                def.id = val;
            } else if (key == "vertex" || key == "vert" || key == "vertexShader" || key == "vertex_shader") {
                def.vertexPath = val;
            } else if (key == "fragment" || key == "frag" || key == "fragmentShader" || key == "fragment_shader") {
                def.fragmentPath = val;
            } else {
                std::string uName = key;
                if (uName.starts_with("uniform.")) uName = uName.substr(8);
                else if (uName.starts_with("param.")) uName = uName.substr(6);

                std::vector<float> nums;
                std::stringstream ss(val);
                float f;
                while (ss >> f) {
                    nums.push_back(f);
                }
                if (!nums.empty()) {
                    if (nums.size() == 1) def.defaultUniforms[uName] = ShaderUniformValue(nums[0]);
                    else if (nums.size() == 2) def.defaultUniforms[uName] = ShaderUniformValue(nums[0], nums[1]);
                    else if (nums.size() == 3) def.defaultUniforms[uName] = ShaderUniformValue(nums[0], nums[1], nums[2]);
                    else if (nums.size() >= 4) def.defaultUniforms[uName] = ShaderUniformValue(nums[0], nums[1], nums[2], nums[3]);
                }
            }
        }
    }

    if (!def.fragmentPath.empty()) {
        std::string vertResolved = m_assetResolver ? m_assetResolver(def.vertexPath) : def.vertexPath;
        std::string fragResolved = m_assetResolver ? m_assetResolver(def.fragmentPath) : def.fragmentPath;
        auto sh = ResourceManager::loadShader(shaderId, vertResolved, fragResolved);
        if (sh && sh->isValid()) {
            std::cout << "[StoryEngine] Successfully loaded custom shader '" << shaderId 
                      << "' from " << path << " (" << def.fragmentPath << ")" << std::endl;
            return true;
        } else {
            std::cerr << "[StoryEngine] Failed to compile custom shader '" << shaderId << "' (" << def.fragmentPath << ")" << std::endl;
            return false;
        }
    }

    std::cerr << "[StoryEngine] Warning: No fragment shader specified in " << path << std::endl;
    return false;
}

bool StoryEngine::switchScript(const std::string& filePath, const std::string& targetLabel) {
    std::string resolved = m_assetResolver ? m_assetResolver(filePath) : filePath;
    auto newCommands = ScriptParser::parseFile(resolved);
    if (newCommands.empty()) {
        std::cerr << "[StoryEngine] Failed to switch script: " << resolved << std::endl;
        return false;
    }

    m_commands = std::move(newCommands);
    m_currentScriptPath = filePath;

    // Переиндексируем метки
    m_labelIndices.clear();
    for (size_t i = 0; i < m_commands.size(); ++i) {
        if (m_commands[i].type == CommandType::Label) {
            m_labelIndices[m_commands[i].targetLabel] = i;
        }
    }

    m_currentCommandIndex = 0;
    if (!targetLabel.empty()) {
        auto it = m_labelIndices.find(targetLabel);
        if (it != m_labelIndices.end()) {
            m_currentCommandIndex = it->second + 1;
        } else {
            std::cerr << "[StoryEngine] Warning: label '" << targetLabel << "' not found in " << resolved << std::endl;
        }
    }

    // При изменении файла сценария все персонажи должны исчезать
    m_activeCharacters.clear();

    m_currentSpeaker.clear();
    m_fullDialogueText.clear();
    m_visibleDialogueText.clear();
    m_charByteIndex = 0;
    m_textTimer = 0.0f;
    m_pauseTimer = 0.0f;
    m_isWaitingForChoice = false;
    m_currentChoices.clear();
    m_isFinished = false;
    m_dialogueMode = DialogueMode::ADV;
    m_nvlLines.clear();
    m_currentBackgroundShader.clear();
    m_currentBackgroundUniforms.clear();

    if (m_scriptChangeHandler) {
        m_scriptChangeHandler(resolved);
    }

    std::cout << "[StoryEngine] Switched to script: " << resolved << " at command " << m_currentCommandIndex << std::endl;
    executeNextCommand();
    return true;
}

bool StoryEngine::loadScript(const std::string& filePath) {
    m_commands = ScriptParser::parseFile(filePath);
    if (m_commands.empty()) {
        std::cerr << "[StoryEngine] Loaded empty script or file not found: " << filePath << std::endl;
        return false;
    }

    m_currentScriptPath = filePath;

    // Индексируем метки перехода (label)
    m_labelIndices.clear();
    for (size_t i = 0; i < m_commands.size(); ++i) {
        if (m_commands[i].type == CommandType::Label) {
            m_labelIndices[m_commands[i].targetLabel] = i;
        }
    }

    reset();
    std::cout << "[StoryEngine] Successfully loaded script: " << filePath << std::endl;
    return true;
}

bool StoryEngine::isPersistentVariable(std::string_view name) {
    if (name.starts_with('$')) name = name.substr(1);
    return name.starts_with("persistent.") || name.starts_with("global.") ||
           name.starts_with("persistent_") || name.starts_with("global_");
}

void StoryEngine::setVariable(const std::string& name, const ScriptValue& val) {
    std::string key = name.starts_with('$') ? name.substr(1) : name;
    if (isPersistentVariable(key)) {
        SaveManager::setPersistentVariable(key, val);
        return;
    }
    m_variables[key] = val;
}

ScriptValue StoryEngine::getVariable(const std::string& name) const {
    std::string key = name.starts_with('$') ? name.substr(1) : name;
    if (isPersistentVariable(key)) {
        return SaveManager::getPersistentVariable(key);
    }
    auto it = m_variables.find(key);
    if (it != m_variables.end()) return it->second;
    return ScriptValue();
}

void StoryEngine::clearVariables() {
    m_variables.clear();
}

bool StoryEngine::evaluateCondition(const std::string& condition) const {
    if (condition.empty()) return true;
    return ExpressionEvaluator::evaluateCondition(condition, [this](const std::string& name) -> ScriptValue {
        return getVariable(name);
    });
}

ScriptValue StoryEngine::evaluateExpression(const std::string& expr) const {
    return ExpressionEvaluator::evaluate(expr, [this](const std::string& name) -> ScriptValue {
        return getVariable(name);
    });
}

std::string StoryEngine::interpolateVariables(std::string_view text) const {
    std::string result;
    result.reserve(text.size());

    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '{') {
            if (i + 1 < text.size() && text[i + 1] == '{') {
                result += "{{";
                i += 2;
                continue;
            }

            size_t closePos = text.find('}', i + 1);
            if (closePos != std::string_view::npos) {
                std::string_view tag = text.substr(i + 1, closePos - (i + 1));
                if (tag.starts_with('$')) {
                    std::string varName(tag.substr(1));
                    result += getVariable(varName).asString();
                    i = closePos + 1;
                    continue;
                }
                if (tag.starts_with("var=")) {
                    std::string varName(tag.substr(4));
                    if (varName.starts_with('$')) varName = varName.substr(1);
                    result += getVariable(varName).asString();
                    i = closePos + 1;
                    continue;
                }

                result += text.substr(i, closePos - i + 1);
                i = closePos + 1;
                continue;
            }
        }

        if (text[i] == '$' && i + 1 < text.size() && text[i + 1] == '{') {
            size_t closePos = text.find('}', i + 2);
            if (closePos != std::string_view::npos) {
                std::string varName(text.substr(i + 2, closePos - (i + 2)));
                if (varName.starts_with('$')) varName = varName.substr(1);
                result += getVariable(varName).asString();
                i = closePos + 1;
                continue;
            }
        }

        result += text[i];
        i++;
    }

    return result;
}

void StoryEngine::reset() {
    m_currentCommandIndex = 0;
    m_currentBackground.clear();
    m_activeCharacters.clear();
    m_currentSpeaker.clear();
    m_fullDialogueText.clear();
    m_visibleDialogueText.clear();
    m_charByteIndex = 0;
    m_textTimer = 0.0f;
    m_pauseTimer = 0.0f;
    m_isWaitingForChoice = false;
    m_currentChoices.clear();
    m_isFinished = false;
    m_dialogueMode = DialogueMode::ADV;
    m_nvlLines.clear();
    m_currentBackgroundShader.clear();
    m_currentBackgroundUniforms.clear();
    m_postProcessSettings.reset();
    m_variables.clear();
    m_dialogueHistory.clear();
}

void StoryEngine::update(float deltaTime) {
    if (m_isWaitingForChoice || m_isFinished) return;

    if (m_pauseTimer > 0.0f) {
        m_pauseTimer -= deltaTime;
        return;
    }

    if (!isTextFullyRevealed()) {
        m_textTimer += deltaTime;
        while (m_textTimer >= m_textSpeed && !isTextFullyRevealed() && m_pauseTimer <= 0.0f) {
            m_textTimer -= m_textSpeed;
            revealNextUtf8Char();
        }
    }
}

void StoryEngine::revealNextUtf8Char() {
    if (m_charByteIndex >= m_fullDialogueText.size()) return;

    // Сначала пропускаем / обрабатываем все теги форматирования в текущей позиции
    while (m_charByteIndex < m_fullDialogueText.size() && m_fullDialogueText[m_charByteIndex] == '{') {
        // Проверяем экранирование {{
        if (m_charByteIndex + 1 < m_fullDialogueText.size() && m_fullDialogueText[m_charByteIndex + 1] == '{') {
            m_charByteIndex += 2;
            m_visibleDialogueText = m_fullDialogueText.substr(0, m_charByteIndex);
            return;
        }

        size_t closePos = m_fullDialogueText.find('}', m_charByteIndex + 1);
        if (closePos == std::string::npos) {
            break;
        }

        std::string_view tag(m_fullDialogueText.data() + m_charByteIndex + 1, closePos - (m_charByteIndex + 1));
        // Проверяем тег паузы {w=0.5} или {w}
        if (tag.starts_with("w=") || tag == "w") {
            float pauseSec = 0.4f;
            if (tag.starts_with("w=")) {
                try {
                    pauseSec = std::stof(std::string(tag.substr(2)));
                } catch (...) {}
            }
            m_charByteIndex = closePos + 1;
            m_pauseTimer = pauseSec;
            m_visibleDialogueText = m_fullDialogueText.substr(0, m_charByteIndex);
            return;
        }

        // Любой другой тег форматирования ({color=...}, {/color}, {b}, {/b} и т.д.)
        // Поглощаем атомарно и продолжаем
        m_charByteIndex = closePos + 1;
    }

    if (m_charByteIndex >= m_fullDialogueText.size()) {
        m_visibleDialogueText = m_fullDialogueText;
        return;
    }

    auto c = static_cast<unsigned char>(m_fullDialogueText[m_charByteIndex]);
    size_t charLen = 1;
    if ((c & 0xE0) == 0xC0) charLen = 2;
    else if ((c & 0xF0) == 0xE0) charLen = 3;
    else if ((c & 0xF8) == 0xF0) charLen = 4;

    m_charByteIndex += charLen;
    if (m_charByteIndex > m_fullDialogueText.size()) {
        m_charByteIndex = m_fullDialogueText.size();
    }
    // Если следующий символ - перенос строки \n, захватываем его сразу
    if (m_charByteIndex < m_fullDialogueText.size() && m_fullDialogueText[m_charByteIndex] == '\n') {
        m_charByteIndex++;
    }

    // Если сразу после символа идут закрывающие теги (например {/color}, {/b}), поглощаем их тоже!
    while (m_charByteIndex < m_fullDialogueText.size() && m_fullDialogueText[m_charByteIndex] == '{') {
        if (m_charByteIndex + 1 < m_fullDialogueText.size() && m_fullDialogueText[m_charByteIndex + 1] == '{') {
            break;
        }
        size_t closePos = m_fullDialogueText.find('}', m_charByteIndex + 1);
        if (closePos == std::string::npos) break;
        std::string_view tag(m_fullDialogueText.data() + m_charByteIndex + 1, closePos - (m_charByteIndex + 1));
        if (tag.starts_with("/")) {
            m_charByteIndex = closePos + 1;
        } else {
            break;
        }
    }

    m_visibleDialogueText = m_fullDialogueText.substr(0, m_charByteIndex);
}

void StoryEngine::revealFullText() {
    m_charByteIndex = m_fullDialogueText.size();
    m_visibleDialogueText = m_fullDialogueText;
    m_pauseTimer = 0.0f;
}

void StoryEngine::nextStep() {
    if (m_isWaitingForChoice) return;

    // 1. Если текст еще печатается, первый клик мгновенно раскрывает всю реплику
    if (!isTextFullyRevealed()) {
        revealFullText();
        return;
    }

    // 2. Если реплика уже дочитана, следующий клик идет дальше по сценарию
    executeNextCommand();
}

void StoryEngine::chooseOption(size_t index) {
    if (!m_isWaitingForChoice || index >= m_currentChoices.size()) return;
    if (!m_currentChoices[index].isEnabled) return;

    ChoiceOption opt = m_currentChoices[index];
    m_isWaitingForChoice = false;
    m_currentChoices.clear();

    if (!opt.targetScript.empty()) {
        switchScript(opt.targetScript, opt.targetLabel);
        return;
    }

    std::string target = opt.targetLabel;
    auto it = m_labelIndices.find(target);
    if (it != m_labelIndices.end()) {
        m_currentCommandIndex = it->second + 1;
    } else {
        std::cerr << "[StoryEngine] Warning: label '" << target << "' not found!" << std::endl;
    }

    executeNextCommand();
}

void StoryEngine::executeNextCommand() {
    while (m_currentCommandIndex < m_commands.size()) {
        const auto& cmd = m_commands[m_currentCommandIndex];

        // Проверка условия выполнения команды
        if (!cmd.condition.empty()) {
            if (!evaluateCondition(cmd.condition)) {
                if (!cmd.elseLabel.empty()) {
                    if (!cmd.elseScript.empty()) {
                        switchScript(cmd.elseScript, cmd.elseLabel);
                        return;
                    }
                    auto it = m_labelIndices.find(cmd.elseLabel);
                    if (it != m_labelIndices.end()) {
                        m_currentCommandIndex = it->second + 1;
                    } else {
                        std::cerr << "[StoryEngine] Else jump target not found: " << cmd.elseLabel << std::endl;
                        m_currentCommandIndex++;
                    }
                    continue;
                }
                m_currentCommandIndex++;
                continue;
            }
        }

        switch (cmd.type) {
            case CommandType::SetVariable: {
                ScriptValue rhs = evaluateExpression(cmd.varExpression);
                std::string varName = cmd.varName.starts_with('$') ? cmd.varName.substr(1) : cmd.varName;

                switch (cmd.varOp) {
                    case VariableOp::Assign:
                        setVariable(varName, rhs);
                        break;
                    case VariableOp::AddAssign: {
                        ScriptValue cur = getVariable(varName);
                        if (cur.isNull()) cur = ScriptValue(0.0);
                        setVariable(varName, cur + rhs);
                        break;
                    }
                    case VariableOp::SubAssign: {
                        ScriptValue cur = getVariable(varName);
                        if (cur.isNull()) cur = ScriptValue(0.0);
                        setVariable(varName, cur - rhs);
                        break;
                    }
                    case VariableOp::MulAssign: {
                        ScriptValue cur = getVariable(varName);
                        if (cur.isNull()) cur = ScriptValue(0.0);
                        setVariable(varName, cur * rhs);
                        break;
                    }
                    case VariableOp::DivAssign: {
                        ScriptValue cur = getVariable(varName);
                        if (cur.isNull()) cur = ScriptValue(0.0);
                        setVariable(varName, cur / rhs);
                        break;
                    }
                }
                std::cout << "[StoryEngine] Variable '" << varName << "' = " << getVariable(varName).asString() << std::endl;
                m_currentCommandIndex++;
                break;
            }
            case CommandType::DefineCharacter: {
                auto& def = m_definedCharacters[cmd.characterName];
                def.id = cmd.characterName;
                def.displayName = cmd.displayName;
                def.expressions["normal"] = cmd.path;
                if (cmd.hasCustomScale) {
                    def.scale = cmd.scale;
                    def.hasCustomScale = true;
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::DefineExpression: {
                auto& def = m_definedCharacters[cmd.characterName];
                def.id = cmd.characterName;
                def.expressions[cmd.expression] = cmd.path;
                m_currentCommandIndex++;
                break;
            }

            case CommandType::LoadCharacter: {
                loadCharacterConfig(cmd.characterName, cmd.path);
                m_currentCommandIndex++;
                break;
            }

            case CommandType::SetBackground:
                if (!cmd.transitionType.empty() && m_transHandler) {
                    m_transHandler(cmd.transitionType, cmd.transitionDuration);
                }
                m_currentBackground = cmd.path;
                m_currentCommandIndex++;
                break;

            case CommandType::Transition:
                if (m_transHandler) {
                    m_transHandler(cmd.transitionType, cmd.transitionDuration);
                }
                m_currentCommandIndex++;
                break;

            case CommandType::JumpScript:
                switchScript(cmd.targetScript, cmd.targetLabel);
                return;

            case CommandType::PlayMusic: {
                if (m_audioHandler) {
                    m_audioHandler({
                        AudioEvent::Type::PlayMusic,
                        cmd.path,
                        cmd.audioLoop,
                        cmd.audioFade,
                        cmd.audioVolume
                    });
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::StopMusic: {
                if (m_audioHandler) {
                    m_audioHandler({
                        AudioEvent::Type::StopMusic,
                        "",
                        false,
                        cmd.audioFade,
                        1.0f
                    });
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::PlaySound: {
                if (m_audioHandler) {
                    m_audioHandler({
                        AudioEvent::Type::PlaySound,
                        cmd.path,
                        false,
                        0.0f,
                        cmd.audioVolume
                    });
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::PlayVoice: {
                if (m_audioHandler) {
                    if (cmd.path.empty()) {
                        m_audioHandler({
                            AudioEvent::Type::StopVoice,
                            "",
                            false,
                            0.0f,
                            1.0f
                        });
                    } else {
                        m_audioHandler({
                            AudioEvent::Type::PlayVoice,
                            cmd.path,
                            false,
                            0.0f,
                            cmd.audioVolume
                        });
                    }
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::PlayAmbient: {
                if (m_audioHandler) {
                    m_audioHandler({
                        AudioEvent::Type::PlayAmbient,
                        cmd.path,
                        cmd.audioLoop,
                        cmd.audioFade,
                        cmd.audioVolume
                    });
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::StopAmbient: {
                if (m_audioHandler) {
                    m_audioHandler({
                        AudioEvent::Type::StopAmbient,
                        "",
                        false,
                        cmd.audioFade,
                        1.0f
                    });
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::ShowCharacter: {
                if (m_definedCharacters.find(cmd.characterName) == m_definedCharacters.end()) {
                    loadCharacterConfig(cmd.characterName);
                }
                auto itDef = m_definedCharacters.find(cmd.characterName);

                auto itActive = m_activeCharacters.find(cmd.characterName);
                bool alreadyActive = (itActive != m_activeCharacters.end());
                glm::vec2 currentPos = alreadyActive ? itActive->second.position : glm::vec2(460.0f, 120.0f);

                glm::vec2 pos;
                if (cmd.hasCustomX && cmd.hasCustomY) {
                    pos = glm::vec2(cmd.posX, cmd.posY);
                } else if (cmd.hasCustomX && !cmd.hasCustomY) {
                    // Высота не указана — перемещаем только по горизонтали (сохраняем текущую Y!)
                    pos = glm::vec2(cmd.posX, currentPos.y);
                } else if (!cmd.positionSlot.empty()) {
                    if (cmd.positionSlot == "left") pos = glm::vec2(140.0f, 120.0f);
                    else if (cmd.positionSlot == "right") pos = glm::vec2(790.0f, 120.0f);
                    else if (cmd.positionSlot == "center") pos = glm::vec2(460.0f, 120.0f);
                    else pos = currentPos;
                } else {
                    pos = currentPos;
                }

                std::string expr = cmd.expression;
                if (expr.empty()) {
                    expr = alreadyActive ? itActive->second.expression : "normal";
                }

                int layer = cmd.layer;
                if (!cmd.hasCustomLayer && alreadyActive) {
                    layer = itActive->second.layer;
                }

                float scale = cmd.hasCustomScale ? cmd.scale : (alreadyActive ? itActive->second.scale : (itDef != m_definedCharacters.end() && itDef->second.hasCustomScale ? itDef->second.scale : 1.0f));
                bool hasCustomScale = cmd.hasCustomScale || (alreadyActive && itActive->second.hasCustomScale) || (itDef != m_definedCharacters.end() && itDef->second.hasCustomScale);

                bool isSilhouette = alreadyActive ? itActive->second.isSilhouette : (itDef != m_definedCharacters.end() ? itDef->second.defaultSilhouette : false);
                glm::vec4 tint = alreadyActive ? itActive->second.tint : glm::vec4(1.0f);
                float sepia = alreadyActive ? itActive->second.sepia : 0.0f;

                std::string customShader = alreadyActive ? itActive->second.customShaderId : (itDef != m_definedCharacters.end() ? itDef->second.defaultShader : "");
                std::unordered_map<std::string, ShaderUniformValue> customUniforms;
                if (alreadyActive) {
                    customUniforms = itActive->second.customUniforms;
                } else if (!customShader.empty()) {
                    auto shIt = m_definedShaders.find(customShader);
                    if (shIt != m_definedShaders.end()) {
                        customUniforms = shIt->second.defaultUniforms;
                    }
                }

                ActiveCharacter ac;
                ac.id = cmd.characterName;
                ac.expression = expr;
                ac.position = pos;
                ac.layer = layer;
                ac.scale = scale;
                ac.hasCustomScale = hasCustomScale;
                ac.positionSlot = cmd.positionSlot;
                ac.tint = tint;
                ac.isSilhouette = isSilhouette;
                ac.silhouetteColor = glm::vec4(0.08f, 0.09f, 0.14f, 1.0f);
                ac.sepia = sepia;
                ac.customShaderId = customShader;
                ac.customUniforms = std::move(customUniforms);

                m_activeCharacters[cmd.characterName] = std::move(ac);
                m_currentCommandIndex++;
                break;
            }

            case CommandType::HideCharacter:
                m_activeCharacters.erase(cmd.characterName);
                m_currentCommandIndex++;
                break;

            case CommandType::Label:
                m_currentCommandIndex++;
                break;

            case CommandType::Jump: {
                auto it = m_labelIndices.find(cmd.targetLabel);
                if (it != m_labelIndices.end()) {
                    m_currentCommandIndex = it->second + 1;
                } else {
                    std::cerr << "[StoryEngine] Jump target not found: " << cmd.targetLabel << std::endl;
                    m_currentCommandIndex++;
                }
                break;
            }

            case CommandType::Effect: {
                if (cmd.effectType == "vignette") {
                    m_postProcessSettings.vignetteIntensity = cmd.effectValue1;
                } else if (cmd.effectType == "sepia") {
                    m_postProcessSettings.sepiaTone = cmd.effectValue1;
                } else if (cmd.effectType == "blur") {
                    m_postProcessSettings.blurStrength = cmd.effectValue1;
                } else if (cmd.effectType == "tint") {
                    m_postProcessSettings.colorTint = glm::vec4(cmd.effectValue1, cmd.effectValue2, cmd.effectValue3, cmd.effectValue4);
                } else if (cmd.effectType == "clear" || cmd.effectType == "reset") {
                    m_postProcessSettings.reset();
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::CharacterEffect: {
                auto it = m_activeCharacters.find(cmd.characterName);
                if (it != m_activeCharacters.end()) {
                    if (cmd.effectType == "silhouette") {
                        it->second.isSilhouette = (cmd.effectValue1 > 0.5f);
                    } else if (cmd.effectType == "tint") {
                        it->second.tint = glm::vec4(cmd.effectValue1, cmd.effectValue2, cmd.effectValue3, cmd.effectValue4);
                    } else if (cmd.effectType == "sepia") {
                        it->second.sepia = cmd.effectValue1;
                    } else if (cmd.effectType == "clear" || cmd.effectType == "reset") {
                        it->second.tint = glm::vec4(1.0f);
                        it->second.isSilhouette = false;
                        it->second.sepia = 0.0f;
                    } else if (cmd.effectType == "shake") {
                        if (m_animHandler) m_animHandler(cmd.characterName, CharacterAnimationEvent::Type::Shake, cmd.effectValue1, cmd.effectValue2);
                    } else if (cmd.effectType == "hop") {
                        if (m_animHandler) m_animHandler(cmd.characterName, CharacterAnimationEvent::Type::Hop, cmd.effectValue1, cmd.effectValue2);
                    } else if (cmd.effectType == "flash") {
                        if (m_animHandler) m_animHandler(cmd.characterName, CharacterAnimationEvent::Type::Flash, cmd.effectValue1, 0.0f);
                    }
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::CharacterShake: {
                if (m_animHandler) {
                    m_animHandler(cmd.characterName, CharacterAnimationEvent::Type::Shake, cmd.effectValue1, cmd.effectValue2);
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::CharacterHop: {
                if (m_animHandler) {
                    m_animHandler(cmd.characterName, CharacterAnimationEvent::Type::Hop, cmd.effectValue1, cmd.effectValue2);
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::CharacterFlash: {
                if (m_animHandler) {
                    m_animHandler(cmd.characterName, CharacterAnimationEvent::Type::Flash, cmd.effectValue1, 0.0f);
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::DialogueModeChange: {
                m_dialogueMode = cmd.dialogueMode;
                if (m_dialogueMode == DialogueMode::ADV) {
                    m_nvlLines.clear();
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::NVLClear: {
                clearNVL();
                m_currentCommandIndex++;
                break;
            }

            case CommandType::LoadShader: {
                loadShaderConfig(cmd.shaderId, cmd.path);
                m_currentCommandIndex++;
                break;
            }

            case CommandType::ShaderScene: {
                m_currentBackgroundShader = cmd.shaderId;
                m_currentBackgroundUniforms.clear();
                if (!cmd.shaderId.empty()) {
                    auto it = m_definedShaders.find(cmd.shaderId);
                    if (it != m_definedShaders.end()) {
                        m_currentBackgroundUniforms = it->second.defaultUniforms;
                    }
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::ShaderCharacter: {
                auto it = m_activeCharacters.find(cmd.characterName);
                if (it != m_activeCharacters.end()) {
                    it->second.customShaderId = cmd.shaderId;
                    it->second.customUniforms.clear();
                    if (!cmd.shaderId.empty()) {
                        auto shIt = m_definedShaders.find(cmd.shaderId);
                        if (shIt != m_definedShaders.end()) {
                            it->second.customUniforms = shIt->second.defaultUniforms;
                        }
                    }
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::ShaderSetUniform: {
                ShaderUniformValue val;
                if (!cmd.uniformValues.empty()) {
                    if (cmd.uniformValues.size() == 1) val = ShaderUniformValue(cmd.uniformValues[0]);
                    else if (cmd.uniformValues.size() == 2) val = ShaderUniformValue(cmd.uniformValues[0], cmd.uniformValues[1]);
                    else if (cmd.uniformValues.size() == 3) val = ShaderUniformValue(cmd.uniformValues[0], cmd.uniformValues[1], cmd.uniformValues[2]);
                    else if (cmd.uniformValues.size() >= 4) val = ShaderUniformValue(cmd.uniformValues[0], cmd.uniformValues[1], cmd.uniformValues[2], cmd.uniformValues[3]);
                }
                if (cmd.characterName.empty() || cmd.characterName == "scene" || cmd.characterName == "bg") {
                    m_currentBackgroundUniforms[cmd.uniformName] = val;
                } else {
                    auto it = m_activeCharacters.find(cmd.characterName);
                    if (it != m_activeCharacters.end()) {
                        it->second.customUniforms[cmd.uniformName] = val;
                    }
                }
                m_currentCommandIndex++;
                break;
            }

            case CommandType::Say: {
                if (m_audioHandler) {
                    m_audioHandler({ AudioEvent::Type::StopVoice, "", false, 0.0f, 1.0f });
                }

                // В NVL-режиме предыдущая прочитанная фраза переносится в накопленный список
                if (m_dialogueMode == DialogueMode::NVL && !m_fullDialogueText.empty()) {
                    m_nvlLines.push_back({m_currentSpeaker, m_fullDialogueText});
                }

                std::string speaker = cmd.speaker;
                if (!speaker.empty()) {
                    if (m_definedCharacters.find(speaker) == m_definedCharacters.end()) {
                        loadCharacterConfig(speaker);
                    }
                    auto it = m_definedCharacters.find(speaker);
                    if (it != m_definedCharacters.end() && !it->second.displayName.empty()) {
                        speaker = it->second.displayName;
                    }
                }
                std::string resolvedSpeaker = Localization::resolve(speaker);
                m_currentSpeaker = interpolateVariables(resolvedSpeaker);

                std::string rawText = Localization::resolve(cmd.text);
                std::string interpolatedText = interpolateVariables(rawText);
                if (m_textFormatter) {
                    m_fullDialogueText = m_textFormatter(interpolatedText);
                } else {
                    m_fullDialogueText = interpolatedText;
                }

                // Запись в историю реплик (бэклог)
                m_dialogueHistory.push_back({m_currentSpeaker, m_fullDialogueText});
                if (m_dialogueHistory.size() > MAX_LOG_ENTRIES) {
                    m_dialogueHistory.erase(m_dialogueHistory.begin());
                }

                m_visibleDialogueText.clear();
                m_charByteIndex = 0;
                m_textTimer = 0.0f;
                m_pauseTimer = 0.0f;
                m_currentCommandIndex++;
                return; // Ждем ввода игрока
            }

            case CommandType::Choice: {
                m_isWaitingForChoice = true;
                m_currentChoices.clear();
                for (const auto& rawOpt : cmd.choices) {
                    bool conditionPassed = true;
                    if (!rawOpt.condition.empty()) {
                        conditionPassed = evaluateCondition(rawOpt.condition);
                    }
                    if (!conditionPassed && rawOpt.conditionMode == ChoiceConditionMode::Hide) {
                        continue; // Условие не выполнено — вариант скрывается
                    }
                    ChoiceOption opt = rawOpt;
                    opt.isEnabled = conditionPassed;
                    opt.text = Localization::resolve(opt.text);
                    opt.text = interpolateVariables(opt.text);
                    m_currentChoices.push_back(std::move(opt));
                }
                if (m_currentChoices.empty()) {
                    m_isWaitingForChoice = false;
                    m_currentCommandIndex++;
                    break;
                }
                m_currentCommandIndex++;
                return; // Ждем выбора игрока
            }
        }
    }

    m_isFinished = true;
    m_currentSpeaker.clear();
    m_fullDialogueText = "[Конец демонстрационного сценария]";
    m_visibleDialogueText = m_fullDialogueText;
    m_charByteIndex = m_fullDialogueText.size();
}

void StoryEngine::captureSaveState(GameSaveState& outState) const {
    outState.scriptPath = m_currentScriptPath;
    outState.commandIndex = m_currentCommandIndex;
    outState.currentBackground = m_currentBackground;
    outState.currentBackgroundShader = m_currentBackgroundShader;
    outState.currentBackgroundUniforms = m_currentBackgroundUniforms;
    outState.currentSpeaker = m_currentSpeaker;
    outState.fullDialogueText = m_fullDialogueText;
    outState.isWaitingForChoice = m_isWaitingForChoice;
    outState.isNVLMode = (m_dialogueMode == DialogueMode::NVL);
    outState.nvlLines.clear();
    for (const auto& l : m_nvlLines) {
        outState.nvlLines.push_back({l.speaker, l.text});
    }

    outState.choices.clear();
    for (const auto& opt : m_currentChoices) {
        outState.choices.push_back({
            opt.text,
            opt.targetLabel,
            opt.targetScript,
            opt.condition,
            opt.isEnabled
        });
    }

    outState.variables = m_variables;

    outState.activeCharacters.clear();
    for (const auto& [name, ch] : m_activeCharacters) {
        CharacterSaveData cd;
        cd.id = name;
        cd.expression = ch.expression;
        cd.posX = ch.position.x;
        cd.posY = ch.position.y;
        cd.positionSlot = ch.positionSlot;
        cd.layer = ch.layer;
        cd.tint = ch.tint;
        cd.isSilhouette = ch.isSilhouette;
        cd.silhouetteColor = ch.silhouetteColor;
        cd.sepia = ch.sepia;
        cd.scale = ch.scale;
        cd.hasCustomScale = ch.hasCustomScale;
        cd.customShaderId = ch.customShaderId;
        cd.customUniforms = ch.customUniforms;
        outState.activeCharacters[name] = std::move(cd);
    }

    outState.postProcess = m_postProcessSettings;

    outState.dialogueHistory.clear();
    for (const auto& entry : m_dialogueHistory) {
        outState.dialogueHistory.push_back({entry.speaker, entry.text});
    }

    outState.previewSpeaker = m_currentSpeaker;
    outState.previewText = m_fullDialogueText;
}

bool StoryEngine::restoreSaveState(const GameSaveState& state) {
    if (state.scriptPath.empty()) return false;

    // 1. Перезагружаем файл сценария
    std::string resolvedScript = m_assetResolver ? m_assetResolver(state.scriptPath) : state.scriptPath;
    m_commands = ScriptParser::parseFile(resolvedScript);
    if (m_commands.empty()) {
        std::cerr << "[StoryEngine] Failed to reload script for save state: " << resolvedScript << std::endl;
        return false;
    }

    m_currentScriptPath = state.scriptPath;

    m_labelIndices.clear();
    for (size_t i = 0; i < m_commands.size(); ++i) {
        if (m_commands[i].type == CommandType::Label) {
            m_labelIndices[m_commands[i].targetLabel] = i;
        }
    }

    // 2. Сканируем файл сценария для восстановления определений персонажей (define character, define expression, load character, load shader)
    m_definedCharacters.clear();
    for (const auto& cmd : m_commands) {
        if (cmd.type == CommandType::DefineCharacter) {
            auto& def = m_definedCharacters[cmd.characterName];
            def.id = cmd.characterName;
            def.displayName = cmd.displayName;
            def.expressions["normal"] = cmd.path;
            if (cmd.hasCustomScale) {
                def.scale = cmd.scale;
                def.hasCustomScale = true;
            }
        } else if (cmd.type == CommandType::DefineExpression) {
            auto& def = m_definedCharacters[cmd.characterName];
            def.id = cmd.characterName;
            def.expressions[cmd.expression] = cmd.path;
        } else if (cmd.type == CommandType::LoadCharacter) {
            loadCharacterConfig(cmd.characterName, cmd.path);
        } else if (cmd.type == CommandType::LoadShader) {
            loadShaderConfig(cmd.shaderId, cmd.path);
        }
    }

    // Для каждого активного персонажа из сохранения гарантируем загрузку конфигурации
    for (const auto& [name, cd] : state.activeCharacters) {
        if (m_definedCharacters.find(name) == m_definedCharacters.end()) {
            loadCharacterConfig(name);
        }
    }

    // 3. Восстанавливаем позицию и данные сценария
    m_currentCommandIndex = state.commandIndex;
    m_currentBackground = state.currentBackground;
    m_currentBackgroundShader = state.currentBackgroundShader;
    m_currentBackgroundUniforms = state.currentBackgroundUniforms;
    if (!m_currentBackgroundShader.empty() && m_definedShaders.find(m_currentBackgroundShader) == m_definedShaders.end()) {
        loadShaderConfig(m_currentBackgroundShader);
    }
    m_currentSpeaker = state.currentSpeaker;
    m_fullDialogueText = state.fullDialogueText;
    m_visibleDialogueText = state.fullDialogueText;
    m_charByteIndex = m_fullDialogueText.size();
    m_textTimer = 0.0f;
    m_pauseTimer = 0.0f;
    m_isWaitingForChoice = state.isWaitingForChoice;
    m_isFinished = false;
    m_dialogueMode = state.isNVLMode ? DialogueMode::NVL : DialogueMode::ADV;
    m_nvlLines.clear();
    for (const auto& l : state.nvlLines) {
        m_nvlLines.push_back({l.speaker, l.text});
    }

    // 3. Восстанавливаем варианты развилки
    m_currentChoices.clear();
    for (const auto& ch : state.choices) {
        ChoiceOption opt;
        opt.text = ch.text;
        opt.targetLabel = ch.targetLabel;
        opt.targetScript = ch.targetScript;
        opt.condition = ch.condition;
        opt.isEnabled = ch.isEnabled;
        m_currentChoices.push_back(std::move(opt));
    }

    // 4. Восстанавливаем переменные
    m_variables = state.variables;

    // 5. Восстанавливаем активных персонажей
    m_activeCharacters.clear();
    for (const auto& [name, cd] : state.activeCharacters) {
        ActiveCharacter ch;
        ch.expression = cd.expression;
        ch.position = glm::vec2(cd.posX, cd.posY);
        ch.positionSlot = cd.positionSlot;
        ch.layer = cd.layer;
        ch.tint = cd.tint;
        ch.isSilhouette = cd.isSilhouette;
        ch.silhouetteColor = cd.silhouetteColor;
        ch.sepia = cd.sepia;
        ch.scale = cd.scale;
        ch.hasCustomScale = cd.hasCustomScale;
        m_activeCharacters[name] = std::move(ch);
    }

    // 6. Восстанавливаем пост-процессинг
    m_postProcessSettings = state.postProcess;

    // 7. Восстанавливаем историю диалогов (бэклог)
    m_dialogueHistory.clear();
    for (const auto& entry : state.dialogueHistory) {
        m_dialogueHistory.push_back({entry.speaker, entry.text});
    }

    std::cout << "[StoryEngine] Successfully restored save state at script '" 
              << m_currentScriptPath << "' command " << m_currentCommandIndex << std::endl;
    return true;
}
