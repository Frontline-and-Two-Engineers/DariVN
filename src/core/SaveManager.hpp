#pragma once

#ifndef DARIVN_SAVEMANAGER_HPP
#define DARIVN_SAVEMANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>
#include "script/ExpressionEvaluator.hpp"
#include "renderer/RenderPipeline.hpp"

// Состояние отдельного персонажа на сцене
struct CharacterSaveData {
    std::string id;
    std::string expression;
    float posX = 0.0f;
    float posY = 0.0f;
    std::string positionSlot;
    int layer = 0;
    glm::vec4 tint{1.0f};
    bool isSilhouette = false;
    glm::vec4 silhouetteColor{0.08f, 0.09f, 0.14f, 1.0f};
    float sepia = 0.0f;
    float scale = 1.0f;
    bool hasCustomScale = false;
};

// Состояние варианта выбора
struct ChoiceSaveData {
    std::string text;
    std::string targetLabel;
    std::string targetScript;
    std::string condition;
};

// Запись в истории диалогов
struct DialogueHistorySaveData {
    std::string speaker;
    std::string text;
};

// Полный снимок состояния игрового процесса
struct GameSaveState {
    int version = 1;
    std::string timestamp;
    std::string previewSpeaker;
    std::string previewText;

    // Сценарий и выполнение
    std::string scriptPath;
    size_t commandIndex = 0;
    std::string currentBackground;
    std::string currentSpeaker;
    std::string fullDialogueText;
    bool isWaitingForChoice = false;
    std::vector<ChoiceSaveData> choices;

    // Переменные сценария
    std::unordered_map<std::string, ScriptValue> variables;

    // Активные персонажи
    std::unordered_map<std::string, CharacterSaveData> activeCharacters;

    // Настройки пост-процессинга
    PostProcessSettings postProcess;

    // Аудиосостояние
    std::string bgmPath;
    float bgmVolume = 1.0f;
    bool bgmLoop = true;
    std::string ambientPath;
    float ambientVolume = 1.0f;
    bool ambientLoop = true;

    // История реплик (бэклог)
    std::vector<DialogueHistorySaveData> dialogueHistory;
};

// Метаданные слота для отображения в интерфейсе сохранения/загрузки
struct SaveSlotMetadata {
    int slotIndex = 0;           // 0 = QuickSave, 1..N = обычные слоты
    bool isQuickSave = false;
    bool exists = false;
    std::string timestamp;
    std::string previewSpeaker;
    std::string previewText;
    std::string screenshotPath;
    std::string saveFilePath;
};

class SaveManager {
public:
    static const int QUICKSAVE_SLOT_INDEX = 0;
    static const int DEFAULT_MAX_SLOTS = 6;

    // Инициализация директории сохранения (по умолчанию "saves/")
    static void setSaveDirectory(const std::string& dirPath);
    [[nodiscard]] static const std::string& getSaveDirectory();

    // Сохранение состояния в указанный слот (slotIndex 0 = QuickSave)
    static bool saveSlot(int slotIndex,
                         const GameSaveState& state,
                         const uint8_t* screenPixels = nullptr,
                         int screenW = 0,
                         int screenH = 0);

    // Загрузка состояния из слота
    static bool loadSlot(int slotIndex, GameSaveState& outState);

    // Удаление файла сохранения и связанного скриншота
    static bool deleteSlot(int slotIndex);

    // Проверка наличия сохранения в слоте
    [[nodiscard]] static bool doesSlotExist(int slotIndex);

    // Получение метаданных конкретного слота
    [[nodiscard]] static SaveSlotMetadata getSlotMetadata(int slotIndex);

    // Получение списка метаданных для всех слотов (от 1 до maxSlots)
    [[nodiscard]] static std::vector<SaveSlotMetadata> getSlotsMetadata(int maxSlots = DEFAULT_MAX_SLOTS);

    // Получение метаданных слота быстрого сохранения
    [[nodiscard]] static SaveSlotMetadata getQuickSaveMetadata();

    // Вспомогательный метод форматирования текущего локального времени
    [[nodiscard]] static std::string getCurrentDateTimeString();

    // Пути к файлам слота
    [[nodiscard]] static std::string getSlotSavePath(int slotIndex);
    [[nodiscard]] static std::string getSlotScreenshotPath(int slotIndex);
};

#endif // DARIVN_SAVEMANAGER_HPP
