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

#ifndef DARIVN_SCRIPTCOMMAND_HPP
#define DARIVN_SCRIPTCOMMAND_HPP

#include <string>
#include <vector>

enum class VariableOp {
    Assign,     // =
    AddAssign,  // +=
    SubAssign,  // -=
    MulAssign,  // *=
    DivAssign   // /=
};

enum class CommandType {
    DefineCharacter,  // define <id> <displayName> <texturePath>
    DefineExpression, // define <id> expression <name> <texturePath>
    Say,              // Реплика (speaker, text)
    SetBackground,    // Установка фона (path)
    ShowCharacter,    // Отобразить персонажа (name, expression, position, layer)
    HideCharacter,    // Скрыть персонажа (name)
    Choice,           // Интерактивный выбор развилки
    Label,            // Метка для перехода
    Jump,             // Переход на метку
    Effect,           // Экранный шейдерный пост-эффект (effect <type> [params...])
    CharacterEffect,  // Эффект персонажа (effect <id> <silhouette|tint|sepia|clear> [params...])
    CharacterShake,   // Тряска персонажа (shake <id> [duration] [intensity])
    CharacterHop,     // Подскок персонажа (hop <id> [duration] [height])
    CharacterFlash,   // Вспышка персонажа (flash <id> [duration])
    Transition,       // Шейдерный переход сцены (transition <type> [duration])
    JumpScript,       // Переход в другой файл сценария (jump script <path> [label])
    LoadCharacter,    // Загрузка конфигурации персонажа (load character <id> [path])
    PlayMusic,        // Воспроизведение музыки (play music <path> [loop] [fade <s>] [volume <v>])
    StopMusic,        // Остановка музыки (stop music [fade <s>])
    PlaySound,        // Звуковой эффект (play sound <path> [volume <v>])
    PlayVoice,        // Озвучка диалога (play voice <path> [volume <v>])
    PlayAmbient,      // Фоновый эмбиент (play ambient <path> [loop] [fade <s>] [volume <v>])
    StopAmbient,      // Остановка эмбиента (stop ambient [fade <s>])
    SetVariable,      // Присваивание/модификация переменной ($var = expr, $var += expr)
    DialogueModeChange, // nvl on, nvl off, mode nvl, mode adv, window fullscreen, window adv
    NVLClear,         // nvl clear
    LoadShader,       // load shader <id> [path]
    ShaderScene,      // shader scene <id> / shader bg <id> (или clear)
    ShaderCharacter,  // shader <character_id> <id> (или clear)
    ShaderSetUniform  // shader set scene <uniform> <values...> / shader <char> set <uniform> <values...>
};

struct ShaderUniformValue {
    int count = 1; // 1 = float, 2 = vec2, 3 = vec3, 4 = vec4, 0 = int
    float f[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int iVal = 0;

    ShaderUniformValue() = default;
    ShaderUniformValue(float val) : count(1) { f[0] = val; }
    ShaderUniformValue(float v1, float v2) : count(2) { f[0] = v1; f[1] = v2; }
    ShaderUniformValue(float v1, float v2, float v3) : count(3) { f[0] = v1; f[1] = v2; f[2] = v3; }
    ShaderUniformValue(float v1, float v2, float v3, float v4) : count(4) { f[0] = v1; f[1] = v2; f[2] = v3; f[3] = v4; }
    ShaderUniformValue(int val) : count(0), iVal(val) {}
};

enum class DialogueMode {
    ADV, // Стандартное диалоговое окно внизу экрана
    NVL  // Полноэкранный текстовый режим (новелла / воспоминания / мысли)
};

enum class ChoiceConditionMode {
    Hide,    // Скрывать вариант при невыполнении условия (по умолчанию)
    Disable  // Отображать вариант серым/неактивным без возможности нажатия
};

struct ChoiceOption {
    std::string text;
    std::string targetLabel;
    std::string targetScript; // опционально: если развилка ведет в другой файл сценария
    std::string condition;    // опционально: условие доступности варианта
    ChoiceConditionMode conditionMode = ChoiceConditionMode::Hide;
    bool isEnabled = true;    // Доступен ли вариант для нажатия (вычисляется в runtime)
};

struct ScriptCommand {
    CommandType type = CommandType::Say;
    ChoiceConditionMode defaultChoiceMode = ChoiceConditionMode::Hide;
    DialogueMode dialogueMode = DialogueMode::ADV;
    std::string characterName;
    std::string speaker;
    std::string displayName;
    std::string expression = "normal";
    std::string text;
    std::string path;
    std::string targetLabel;
    std::string targetScript; // путь к файлу при jump script

    // Параметры перехода между сценами
    std::string transitionType;
    float transitionDuration = 1.0f;

    // Свободные координаты или слот
    bool hasCustomCoords = false; // true если указаны и X и Y
    bool hasCustomX = false;      // указан ли X
    bool hasCustomY = false;      // указана ли высота Y
    float posX = 0.0f;
    float posY = 0.0f;
    std::string positionSlot;     // "left", "center", "right"
    int layer = 0;                // Слой отрисовки (Z-index)
    bool hasCustomLayer = false;  // Был ли явно указан layer в команде

    // Масштаб персонажа (scale 0.8 / scale 1.0)
    float scale = 1.0f;
    bool hasCustomScale = false;

    // Параметры экранного пост-эффекта
    std::string effectType; // "vignette", "sepia", "blur", "tint", "clear"
    float effectValue1 = 0.0f;
    float effectValue2 = 0.0f;
    float effectValue3 = 0.0f;
    float effectValue4 = 1.0f;

    // Параметры воспроизведения звука/музыки
    bool audioLoop = true;
    float audioFade = 0.0f;
    float audioVolume = 1.0f;

    // Условия и переменные
    std::string condition;        // Условие выполнения команды (если не пустое, проверяется перед запуском)
    std::string varName;          // Имя переменной (без '$')
    VariableOp varOp = VariableOp::Assign;
    std::string varExpression;    // Правая часть выражения присваивания
    std::string elseLabel;        // Опциональная метка перехода при невыполнении условия
    std::string elseScript;       // Опциональный скрипт перехода при невыполнении условия

    // Пользовательские шейдеры
    std::string shaderId;
    std::string uniformName;
    std::vector<float> uniformValues;

    std::vector<ChoiceOption> choices;
};

#endif //DARIVN_SCRIPTCOMMAND_HPP
