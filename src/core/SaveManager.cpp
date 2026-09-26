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

#include "SaveManager.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cctype>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace {

std::string s_saveDir = "saves";

// =============================================================================
// Компактный и надежный парсер/сериализатор JSON без внешних библиотек
// =============================================================================

enum class JsonType { Null, Bool, Number, String, Array, Object };

struct JsonValue {
    JsonType type = JsonType::Null;
    bool boolVal = false;
    double numVal = 0.0;
    std::string strVal;
    std::vector<JsonValue> arrVal;
    std::unordered_map<std::string, JsonValue> objVal;

    [[nodiscard]] bool isNull() const { return type == JsonType::Null; }
    [[nodiscard]] bool isBool() const { return type == JsonType::Bool; }
    [[nodiscard]] bool isNumber() const { return type == JsonType::Number; }
    [[nodiscard]] bool isString() const { return type == JsonType::String; }
    [[nodiscard]] bool isArray() const { return type == JsonType::Array; }
    [[nodiscard]] bool isObject() const { return type == JsonType::Object; }

    [[nodiscard]] bool asBool(bool def = false) const { return isBool() ? boolVal : def; }
    [[nodiscard]] double asDouble(double def = 0.0) const { return isNumber() ? numVal : def; }
    [[nodiscard]] float asFloat(float def = 0.0f) const { return isNumber() ? static_cast<float>(numVal) : def; }
    [[nodiscard]] int asInt(int def = 0) const { return isNumber() ? static_cast<int>(numVal) : def; }
    [[nodiscard]] size_t asSizeT(size_t def = 0) const { return isNumber() ? static_cast<size_t>(numVal) : def; }
    [[nodiscard]] const std::string& asString(const std::string& def = "") const { return isString() ? strVal : def; }

    [[nodiscard]] const JsonValue* get(const std::string& key) const {
        if (!isObject()) return nullptr;
        auto it = objVal.find(key);
        return (it != objVal.end()) ? &it->second : nullptr;
    }
};

void skipWhitespace(std::string_view s, size_t& i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
        i++;
    }
}

std::string escapeJsonString(std::string_view s) {
    std::string res;
    res.reserve(s.size() + 16);
    res += '"';
    for (char c : s) {
        switch (c) {
            case '"':  res += "\\\""; break;
            case '\\': res += "\\\\"; break;
            case '\b': res += "\\b"; break;
            case '\f': res += "\\f"; break;
            case '\n': res += "\\n"; break;
            case '\r': res += "\\r"; break;
            case '\t': res += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    res += buf;
                } else {
                    res += c;
                }
                break;
        }
    }
    res += '"';
    return res;
}

bool parseJsonValue(std::string_view s, size_t& i, JsonValue& out);

bool parseJsonString(std::string_view s, size_t& i, std::string& out) {
    if (i >= s.size() || s[i] != '"') return false;
    i++;
    out.clear();
    while (i < s.size()) {
        char c = s[i++];
        if (c == '"') return true;
        if (c == '\\') {
            if (i >= s.size()) return false;
            char esc = s[i++];
            switch (esc) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    if (i + 4 <= s.size()) {
                        // Для сохранения/загрузки достаточно простого hex парсинга
                        std::string hexStr(s.substr(i, 4));
                        i += 4;
                        try {
                            uint32_t cp = std::stoul(hexStr, nullptr, 16);
                            if (cp < 0x80) {
                                out += static_cast<char>(cp);
                            } else if (cp < 0x800) {
                                out += static_cast<char>(0xC0 | (cp >> 6));
                                out += static_cast<char>(0x80 | (cp & 0x3F));
                            } else {
                                out += static_cast<char>(0xE0 | (cp >> 12));
                                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                                out += static_cast<char>(0x80 | (cp & 0x3F));
                            }
                        } catch (...) {}
                    }
                    break;
                }
                default: out += esc; break;
            }
        } else {
            out += c;
        }
    }
    return false;
}

bool parseJsonNumber(std::string_view s, size_t& i, double& out) {
    size_t start = i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) i++;
    bool hasDigits = false;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '-' || s[i] == '+')) {
        if (std::isdigit(static_cast<unsigned char>(s[i]))) hasDigits = true;
        i++;
    }
    if (!hasDigits) return false;
    std::string numStr(s.substr(start, i - start));
    try {
        out = std::stod(numStr);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseJsonArray(std::string_view s, size_t& i, JsonValue& out) {
    if (i >= s.size() || s[i] != '[') return false;
    i++;
    out.type = JsonType::Array;
    out.arrVal.clear();
    skipWhitespace(s, i);
    if (i < s.size() && s[i] == ']') {
        i++;
        return true;
    }

    while (i < s.size()) {
        JsonValue elem;
        if (!parseJsonValue(s, i, elem)) return false;
        out.arrVal.push_back(std::move(elem));
        skipWhitespace(s, i);
        if (i < s.size() && s[i] == ',') {
            i++;
            skipWhitespace(s, i);
            continue;
        }
        if (i < s.size() && s[i] == ']') {
            i++;
            return true;
        }
        break;
    }
    return false;
}

bool parseJsonObject(std::string_view s, size_t& i, JsonValue& out) {
    if (i >= s.size() || s[i] != '{') return false;
    i++;
    out.type = JsonType::Object;
    out.objVal.clear();
    skipWhitespace(s, i);
    if (i < s.size() && s[i] == '}') {
        i++;
        return true;
    }

    while (i < s.size()) {
        skipWhitespace(s, i);
        std::string key;
        if (!parseJsonString(s, i, key)) return false;
        skipWhitespace(s, i);
        if (i >= s.size() || s[i] != ':') return false;
        i++;
        skipWhitespace(s, i);
        JsonValue val;
        if (!parseJsonValue(s, i, val)) return false;
        out.objVal[key] = std::move(val);
        skipWhitespace(s, i);
        if (i < s.size() && s[i] == ',') {
            i++;
            skipWhitespace(s, i);
            continue;
        }
        if (i < s.size() && s[i] == '}') {
            i++;
            return true;
        }
        break;
    }
    return false;
}

bool parseJsonValue(std::string_view s, size_t& i, JsonValue& out) {
    skipWhitespace(s, i);
    if (i >= s.size()) return false;

    char c = s[i];
    if (c == '"') {
        out.type = JsonType::String;
        return parseJsonString(s, i, out.strVal);
    }
    if (c == '{') {
        return parseJsonObject(s, i, out);
    }
    if (c == '[') {
        return parseJsonArray(s, i, out);
    }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
        out.type = JsonType::Number;
        return parseJsonNumber(s, i, out.numVal);
    }
    if (s.substr(i, 4) == "true") {
        out.type = JsonType::Bool;
        out.boolVal = true;
        i += 4;
        return true;
    }
    if (s.substr(i, 5) == "false") {
        out.type = JsonType::Bool;
        out.boolVal = false;
        i += 5;
        return true;
    }
    if (s.substr(i, 4) == "null") {
        out.type = JsonType::Null;
        i += 4;
        return true;
    }
    return false;
}

// Сериализация состояния GameSaveState в JSON-строку
std::string serializeSaveStateToJson(const GameSaveState& state) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": " << state.version << ",\n";
    ss << "  \"timestamp\": " << escapeJsonString(state.timestamp) << ",\n";
    ss << "  \"previewSpeaker\": " << escapeJsonString(state.previewSpeaker) << ",\n";
    ss << "  \"previewText\": " << escapeJsonString(state.previewText) << ",\n";

    // Скрипт
    ss << "  \"script\": {\n";
    ss << "    \"path\": " << escapeJsonString(state.scriptPath) << ",\n";
    ss << "    \"commandIndex\": " << state.commandIndex << ",\n";
    ss << "    \"background\": " << escapeJsonString(state.currentBackground) << ",\n";
    ss << "    \"speaker\": " << escapeJsonString(state.currentSpeaker) << ",\n";
    ss << "    \"fullDialogue\": " << escapeJsonString(state.fullDialogueText) << ",\n";
    ss << "    \"isWaitingForChoice\": " << (state.isWaitingForChoice ? "true" : "false") << ",\n";
    ss << "    \"choices\": [\n";
    for (size_t c = 0; c < state.choices.size(); ++c) {
        const auto& ch = state.choices[c];
        ss << "      {\"text\": " << escapeJsonString(ch.text)
           << ", \"targetLabel\": " << escapeJsonString(ch.targetLabel)
           << ", \"targetScript\": " << escapeJsonString(ch.targetScript)
           << ", \"condition\": " << escapeJsonString(ch.condition) << "}"
           << (c + 1 < state.choices.size() ? ",\n" : "\n");
    }
    ss << "    ]\n";
    ss << "  },\n";

    // Переменные
    ss << "  \"variables\": {\n";
    size_t varIdx = 0;
    for (const auto& [varName, varVal] : state.variables) {
        ss << "    " << escapeJsonString(varName) << ": {";
        ss << "\"type\": ";
        if (varVal.isNull()) ss << "\"null\", \"val\": null";
        else if (varVal.isBool()) ss << "\"bool\", \"val\": " << (varVal.asBool() ? "true" : "false");
        else if (varVal.isNumber()) ss << "\"number\", \"val\": " << varVal.asNumber();
        else ss << "\"string\", \"val\": " << escapeJsonString(varVal.asString());
        ss << "}" << (++varIdx < state.variables.size() ? ",\n" : "\n");
    }
    ss << "  },\n";

    // Персонажи
    ss << "  \"activeCharacters\": {\n";
    size_t charIdx = 0;
    for (const auto& [name, ch] : state.activeCharacters) {
        ss << "    " << escapeJsonString(name) << ": {\n";
        ss << "      \"expression\": " << escapeJsonString(ch.expression) << ",\n";
        ss << "      \"posX\": " << ch.posX << ",\n";
        ss << "      \"posY\": " << ch.posY << ",\n";
        ss << "      \"slot\": " << escapeJsonString(ch.positionSlot) << ",\n";
        ss << "      \"layer\": " << ch.layer << ",\n";
        ss << "      \"tint\": [" << ch.tint.r << ", " << ch.tint.g << ", " << ch.tint.b << ", " << ch.tint.a << "],\n";
        ss << "      \"isSilhouette\": " << (ch.isSilhouette ? "true" : "false") << ",\n";
        ss << "      \"silhouetteColor\": [" << ch.silhouetteColor.r << ", " << ch.silhouetteColor.g << ", " << ch.silhouetteColor.b << ", " << ch.silhouetteColor.a << "],\n";
        ss << "      \"sepia\": " << ch.sepia << ",\n";
        ss << "      \"scale\": " << ch.scale << ",\n";
        ss << "      \"hasCustomScale\": " << (ch.hasCustomScale ? "true" : "false") << "\n";
        ss << "    }" << (++charIdx < state.activeCharacters.size() ? ",\n" : "\n");
    }
    ss << "  },\n";

    // Пост-процессинг
    ss << "  \"postProcess\": {\n";
    ss << "    \"vignette\": " << state.postProcess.vignetteIntensity << ",\n";
    ss << "    \"sepia\": " << state.postProcess.sepiaTone << ",\n";
    ss << "    \"blur\": " << state.postProcess.blurStrength << ",\n";
    ss << "    \"tint\": [" << state.postProcess.colorTint.r << ", " << state.postProcess.colorTint.g << ", " << state.postProcess.colorTint.b << ", " << state.postProcess.colorTint.a << "]\n";
    ss << "  },\n";

    // Аудио
    ss << "  \"audio\": {\n";
    ss << "    \"bgmPath\": " << escapeJsonString(state.bgmPath) << ",\n";
    ss << "    \"bgmVolume\": " << state.bgmVolume << ",\n";
    ss << "    \"bgmLoop\": " << (state.bgmLoop ? "true" : "false") << ",\n";
    ss << "    \"ambientPath\": " << escapeJsonString(state.ambientPath) << ",\n";
    ss << "    \"ambientVolume\": " << state.ambientVolume << ",\n";
    ss << "    \"ambientLoop\": " << (state.ambientLoop ? "true" : "false") << "\n";
    ss << "  },\n";

    // История диалогов
    ss << "  \"dialogueHistory\": [\n";
    for (size_t h = 0; h < state.dialogueHistory.size(); ++h) {
        const auto& entry = state.dialogueHistory[h];
        ss << "    {\"speaker\": " << escapeJsonString(entry.speaker)
           << ", \"text\": " << escapeJsonString(entry.text) << "}"
           << (h + 1 < state.dialogueHistory.size() ? ",\n" : "\n");
    }
    ss << "  ]\n";

    ss << "}\n";
    return ss.str();
}

// Десериализация JsonValue в GameSaveState
bool deserializeJsonToSaveState(const JsonValue& root, GameSaveState& out) {
    if (!root.isObject()) return false;

    if (auto* v = root.get("version")) out.version = v->asInt(1);
    if (auto* ts = root.get("timestamp")) out.timestamp = ts->asString();
    if (auto* ps = root.get("previewSpeaker")) out.previewSpeaker = ps->asString();
    if (auto* pt = root.get("previewText")) out.previewText = pt->asString();

    if (auto* sc = root.get("script")) {
        if (auto* p = sc->get("path")) out.scriptPath = p->asString();
        if (auto* ci = sc->get("commandIndex")) out.commandIndex = ci->asSizeT();
        if (auto* bg = sc->get("background")) out.currentBackground = bg->asString();
        if (auto* sp = sc->get("speaker")) out.currentSpeaker = sp->asString();
        if (auto* fd = sc->get("fullDialogue")) out.fullDialogueText = fd->asString();
        if (auto* w = sc->get("isWaitingForChoice")) out.isWaitingForChoice = w->asBool();
        if (auto* chs = sc->get("choices")) {
            if (chs->isArray()) {
                out.choices.clear();
                for (const auto& cVal : chs->arrVal) {
                    ChoiceSaveData cd;
                    if (auto* t = cVal.get("text")) cd.text = t->asString();
                    if (auto* tl = cVal.get("targetLabel")) cd.targetLabel = tl->asString();
                    if (auto* ts = cVal.get("targetScript")) cd.targetScript = ts->asString();
                    if (auto* cond = cVal.get("condition")) cd.condition = cond->asString();
                    out.choices.push_back(std::move(cd));
                }
            }
        }
    }

    if (auto* vars = root.get("variables")) {
        if (vars->isObject()) {
            out.variables.clear();
            for (const auto& [varName, valObj] : vars->objVal) {
                std::string typeStr;
                if (auto* t = valObj.get("type")) typeStr = t->asString();
                auto* v = valObj.get("val");
                if (!v || typeStr == "null" || v->isNull()) {
                    out.variables[varName] = ScriptValue();
                } else if (typeStr == "bool" || v->isBool()) {
                    out.variables[varName] = ScriptValue(v->asBool());
                } else if (typeStr == "number" || v->isNumber()) {
                    out.variables[varName] = ScriptValue(v->asDouble());
                } else {
                    out.variables[varName] = ScriptValue(v->asString());
                }
            }
        }
    }

    if (auto* chars = root.get("activeCharacters")) {
        if (chars->isObject()) {
            out.activeCharacters.clear();
            for (const auto& [charName, cObj] : chars->objVal) {
                CharacterSaveData cd;
                cd.id = charName;
                if (auto* e = cObj.get("expression")) cd.expression = e->asString();
                if (auto* px = cObj.get("posX")) cd.posX = px->asFloat();
                if (auto* py = cObj.get("posY")) cd.posY = py->asFloat();
                if (auto* sl = cObj.get("slot")) cd.positionSlot = sl->asString();
                if (auto* l = cObj.get("layer")) cd.layer = l->asInt();
                if (auto* s = cObj.get("scale")) cd.scale = s->asFloat(1.0f);
                if (auto* hcs = cObj.get("hasCustomScale")) cd.hasCustomScale = hcs->asBool();
                if (auto* sep = cObj.get("sepia")) cd.sepia = sep->asFloat();
                if (auto* sil = cObj.get("isSilhouette")) cd.isSilhouette = sil->asBool();
                if (auto* t = cObj.get("tint")) {
                    if (t->isArray() && t->arrVal.size() >= 4) {
                        cd.tint = glm::vec4(t->arrVal[0].asFloat(1.0f),
                                            t->arrVal[1].asFloat(1.0f),
                                            t->arrVal[2].asFloat(1.0f),
                                            t->arrVal[3].asFloat(1.0f));
                    }
                }
                if (auto* sc = cObj.get("silhouetteColor")) {
                    if (sc->isArray() && sc->arrVal.size() >= 4) {
                        cd.silhouetteColor = glm::vec4(sc->arrVal[0].asFloat(0.08f),
                                                       sc->arrVal[1].asFloat(0.09f),
                                                       sc->arrVal[2].asFloat(0.14f),
                                                       sc->arrVal[3].asFloat(1.0f));
                    }
                }
                out.activeCharacters[charName] = std::move(cd);
            }
        }
    }

    if (auto* pp = root.get("postProcess")) {
        if (auto* v = pp->get("vignette")) out.postProcess.vignetteIntensity = v->asFloat();
        if (auto* s = pp->get("sepia")) out.postProcess.sepiaTone = s->asFloat();
        if (auto* b = pp->get("blur")) out.postProcess.blurStrength = b->asFloat();
        if (auto* t = pp->get("tint")) {
            if (t->isArray() && t->arrVal.size() >= 4) {
                out.postProcess.colorTint = glm::vec4(t->arrVal[0].asFloat(1.0f),
                                                      t->arrVal[1].asFloat(1.0f),
                                                      t->arrVal[2].asFloat(1.0f),
                                                      t->arrVal[3].asFloat(1.0f));
            }
        }
    }

    if (auto* au = root.get("audio")) {
        if (auto* bp = au->get("bgmPath")) out.bgmPath = bp->asString();
        if (auto* bv = au->get("bgmVolume")) out.bgmVolume = bv->asFloat(1.0f);
        if (auto* bl = au->get("bgmLoop")) out.bgmLoop = bl->asBool(true);
        if (auto* ap = au->get("ambientPath")) out.ambientPath = ap->asString();
        if (auto* av = au->get("ambientVolume")) out.ambientVolume = av->asFloat(1.0f);
        if (auto* al = au->get("ambientLoop")) out.ambientLoop = al->asBool(true);
    }

    if (auto* dh = root.get("dialogueHistory")) {
        if (dh->isArray()) {
            out.dialogueHistory.clear();
            for (const auto& entry : dh->arrVal) {
                DialogueHistorySaveData item;
                if (auto* sp = entry.get("speaker")) item.speaker = sp->asString();
                if (auto* tx = entry.get("text")) item.text = tx->asString();
                out.dialogueHistory.push_back(std::move(item));
            }
        }
    }

    return true;
}

} // anonymous namespace

// =============================================================================
// Реализация методов класса SaveManager
// =============================================================================

void SaveManager::setSaveDirectory(const std::string& dirPath) {
    s_saveDir = dirPath;
}

const std::string& SaveManager::getSaveDirectory() {
    return s_saveDir;
}

std::string SaveManager::getSlotSavePath(int slotIndex) {
    std::filesystem::path dir(s_saveDir);
    if (slotIndex == QUICKSAVE_SLOT_INDEX) {
        return (dir / "quicksave.json").string();
    }
    return (dir / ("slot_" + std::to_string(slotIndex) + ".json")).string();
}

std::string SaveManager::getSlotScreenshotPath(int slotIndex) {
    std::filesystem::path dir(s_saveDir);
    if (slotIndex == QUICKSAVE_SLOT_INDEX) {
        return (dir / "quicksave.png").string();
    }
    return (dir / ("slot_" + std::to_string(slotIndex) + ".png")).string();
}

std::string SaveManager::getCurrentDateTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tmNow{};
#if defined(_WIN32)
    localtime_s(&tmNow, &tt);
#else
    localtime_r(&tt, &tmNow);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", &tmNow);
    return std::string(buf);
}

bool SaveManager::saveSlot(int slotIndex,
                           const GameSaveState& state,
                           const uint8_t* screenPixels,
                           int screenW,
                           int screenH) {
    try {
        std::filesystem::create_directories(s_saveDir);
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] Failed to create directory: " << e.what() << std::endl;
        return false;
    }

    GameSaveState finalState = state;
    if (finalState.timestamp.empty()) {
        finalState.timestamp = getCurrentDateTimeString();
    }

    std::string jsonStr = serializeSaveStateToJson(finalState);
    std::string savePath = getSlotSavePath(slotIndex);

    std::ofstream file(savePath);
    if (!file.is_open()) {
        std::cerr << "[SaveManager] Failed to open save file for writing: " << savePath << std::endl;
        return false;
    }
    file << jsonStr;
    file.close();

    // 2. Сохранение скриншота-превью (если переданы пиксели буфера кадра)
    if (screenPixels && screenW > 0 && screenH > 0) {
        int thumbW = 256;
        int thumbH = 144;
        std::vector<uint8_t> thumbRGB(thumbW * thumbH * 3);

        // Даунсемплинг с вертикальным отражением (OpenGL читает снизу-вверх, а PNG ожидает сверху-вниз)
        for (int y = 0; y < thumbH; ++y) {
            int srcY = screenH - 1 - (y * screenH / thumbH);
            if (srcY < 0) srcY = 0;
            if (srcY >= screenH) srcY = screenH - 1;

            for (int x = 0; x < thumbW; ++x) {
                int srcX = x * screenW / thumbW;
                if (srcX >= screenW) srcX = screenW - 1;

                size_t srcIdx = static_cast<size_t>((srcY * screenW + srcX) * 3);
                size_t dstIdx = static_cast<size_t>((y * thumbW + x) * 3);

                thumbRGB[dstIdx + 0] = screenPixels[srcIdx + 0];
                thumbRGB[dstIdx + 1] = screenPixels[srcIdx + 1];
                thumbRGB[dstIdx + 2] = screenPixels[srcIdx + 2];
            }
        }

        std::string screenPath = getSlotScreenshotPath(slotIndex);
        if (!stbi_write_png(screenPath.c_str(), thumbW, thumbH, 3, thumbRGB.data(), thumbW * 3)) {
            std::cerr << "[SaveManager] Warning: failed to write screenshot to: " << screenPath << std::endl;
        }
    }

    std::cout << "[SaveManager] Successfully saved slot " << slotIndex << " to: " << savePath << std::endl;
    return true;
}

bool SaveManager::loadSlot(int slotIndex, GameSaveState& outState) {
    std::string savePath = getSlotSavePath(slotIndex);
    if (!std::filesystem::exists(savePath)) {
        std::cerr << "[SaveManager] Save file does not exist: " << savePath << std::endl;
        return false;
    }

    std::ifstream file(savePath);
    if (!file.is_open()) {
        std::cerr << "[SaveManager] Failed to open save file for reading: " << savePath << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    size_t i = 0;
    JsonValue root;
    if (!parseJsonValue(content, i, root)) {
        std::cerr << "[SaveManager] Failed to parse JSON from: " << savePath << std::endl;
        return false;
    }

    if (!deserializeJsonToSaveState(root, outState)) {
        std::cerr << "[SaveManager] Failed to deserialize save state from: " << savePath << std::endl;
        return false;
    }

    std::cout << "[SaveManager] Successfully loaded slot " << slotIndex << " from: " << savePath << std::endl;
    return true;
}

bool SaveManager::deleteSlot(int slotIndex) {
    bool deletedAny = false;
    std::string savePath = getSlotSavePath(slotIndex);
    if (std::filesystem::exists(savePath)) {
        std::filesystem::remove(savePath);
        deletedAny = true;
    }
    std::string screenPath = getSlotScreenshotPath(slotIndex);
    if (std::filesystem::exists(screenPath)) {
        std::filesystem::remove(screenPath);
        deletedAny = true;
    }
    return deletedAny;
}

bool SaveManager::doesSlotExist(int slotIndex) {
    return std::filesystem::exists(getSlotSavePath(slotIndex));
}

SaveSlotMetadata SaveManager::getSlotMetadata(int slotIndex) {
    SaveSlotMetadata meta;
    meta.slotIndex = slotIndex;
    meta.isQuickSave = (slotIndex == QUICKSAVE_SLOT_INDEX);
    meta.saveFilePath = getSlotSavePath(slotIndex);
    meta.screenshotPath = getSlotScreenshotPath(slotIndex);

    if (!std::filesystem::exists(meta.saveFilePath)) {
        meta.exists = false;
        return meta;
    }

    meta.exists = true;

    // Быстрое считывание превью метаданных без полной загрузки всех персонажей/переменных
    std::ifstream file(meta.saveFilePath);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        size_t i = 0;
        JsonValue root;
        if (parseJsonValue(content, i, root) && root.isObject()) {
            if (auto* ts = root.get("timestamp")) meta.timestamp = ts->asString();
            if (auto* sp = root.get("previewSpeaker")) meta.previewSpeaker = sp->asString();
            if (auto* pt = root.get("previewText")) meta.previewText = pt->asString();
        }
    }

    if (!std::filesystem::exists(meta.screenshotPath)) {
        meta.screenshotPath.clear();
    }

    return meta;
}

std::vector<SaveSlotMetadata> SaveManager::getSlotsMetadata(int maxSlots) {
    std::vector<SaveSlotMetadata> result;
    result.reserve(maxSlots);
    for (int s = 1; s <= maxSlots; ++s) {
        result.push_back(getSlotMetadata(s));
    }
    return result;
}

SaveSlotMetadata SaveManager::getQuickSaveMetadata() {
    return getSlotMetadata(QUICKSAVE_SLOT_INDEX);
}
