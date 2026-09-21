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
    SetVariable       // Присваивание/модификация переменной ($var = expr, $var += expr)
};

struct ChoiceOption {
    std::string text;
    std::string targetLabel;
    std::string targetScript; // опционально: если развилка ведет в другой файл сценария
    std::string condition;    // опционально: условие доступности варианта
};

struct ScriptCommand {
    CommandType type = CommandType::Say;
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

    std::vector<ChoiceOption> choices;
};

#endif //DARIVN_SCRIPTCOMMAND_HPP
