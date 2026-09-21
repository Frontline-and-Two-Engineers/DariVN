#pragma once

#ifndef DARIVN_STORYENGINE_HPP
#define DARIVN_STORYENGINE_HPP

#include "ScriptCommand.hpp"
#include "ExpressionEvaluator.hpp"
#include "renderer/RenderPipeline.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>

// Определение персонажа, созданного в сценарии
struct CharacterDefinition {
    std::string id;
    std::string displayName;
    std::unordered_map<std::string, std::string> expressions; // exprName -> texturePath
    float scale = 1.0f;
    bool hasCustomScale = false;
    bool defaultSilhouette = false;
};

// Состояние активного персонажа на сцене
struct ActiveCharacter {
    std::string id;
    std::string expression = "normal";
    glm::vec2 position{460.0f, 120.0f};
    int layer = 0; // Слой отрисовки
    float scale = 1.0f;
    bool hasCustomScale = false;
    std::string positionSlot;

    // Визуальные эффекты персонажа
    glm::vec4 tint = glm::vec4(1.0f);
    bool isSilhouette = false;
    glm::vec4 silhouetteColor = glm::vec4(0.08f, 0.09f, 0.14f, 1.0f);
    float sepia = 0.0f;
};

struct CharacterAnimationEvent {
    enum class Type { Shake, Hop, Flash };
};

class StoryEngine {
public:
    using AnimationEventHandler = std::function<void(const std::string& charId, CharacterAnimationEvent::Type type, float p1, float p2)>;
    using TransitionEventHandler = std::function<void(const std::string& transitionType, float duration)>;
    using AssetResolver = std::function<std::string(const std::string&)>;
    using ScriptChangeHandler = std::function<void(const std::string& newScript)>;

    struct AudioEvent {
        enum class Type {
            PlayMusic, StopMusic,
            PlaySound,
            PlayVoice, StopVoice,
            PlayAmbient, StopAmbient
        } type;
        std::string path;
        bool loop = true;
        float fade = 0.0f;
        float volume = 1.0f;
    };
    using AudioEventHandler = std::function<void(const AudioEvent& event)>;

    StoryEngine();
    ~StoryEngine() = default;

    void setAnimationEventHandler(AnimationEventHandler handler) { m_animHandler = std::move(handler); }
    void setTransitionEventHandler(TransitionEventHandler handler) { m_transHandler = std::move(handler); }
    void setAssetResolver(AssetResolver resolver) { m_assetResolver = std::move(resolver); }
    void setScriptChangeHandler(ScriptChangeHandler handler) { m_scriptChangeHandler = std::move(handler); }
    void setAudioEventHandler(AudioEventHandler handler) { m_audioHandler = std::move(handler); }

    // Загрузка отдельного файла конфигурации персонажа по ID (например assets/characters/dari.vn)
    bool loadCharacterConfig(const std::string& charId, const std::string& specificPath = "");

    // Переключение на другой файл сценария на лету
    bool switchScript(const std::string& filePath, const std::string& targetLabel = "");

    // Загрузка файла сценария
    bool loadScript(const std::string& filePath);

    // Обновление анимации текста ("печатная машинка")
    void update(float deltaTime);

    // Действие по клику игрока / пробелу
    void nextStep();

    // Выбор варианта в развилке
    void chooseOption(size_t index);

    // Сброс сценария в начало
    void reset();

    // Состояние сцены
    [[nodiscard]] const std::string& getCurrentSpeaker() const { return m_currentSpeaker; }
    [[nodiscard]] const std::string& getVisibleText() const { return m_visibleDialogueText; }
    [[nodiscard]] const std::string& getCurrentBackground() const { return m_currentBackground; }
    
    // Персонажи
    [[nodiscard]] const std::unordered_map<std::string, CharacterDefinition>& getDefinedCharacters() const { return m_definedCharacters; }
    [[nodiscard]] const std::unordered_map<std::string, ActiveCharacter>& getActiveCharacters() const { return m_activeCharacters; }

    [[nodiscard]] bool isWaitingForChoice() const { return m_isWaitingForChoice; }
    [[nodiscard]] const std::vector<ChoiceOption>& getCurrentChoices() const { return m_currentChoices; }
    [[nodiscard]] bool isFinished() const { return m_isFinished; }

    [[nodiscard]] bool isTextFullyRevealed() const { return m_charByteIndex >= m_fullDialogueText.size(); }
    [[nodiscard]] bool isWaitingForNextClick() const {
        return !m_isFinished && !m_isWaitingForChoice && isTextFullyRevealed();
    }
    void revealFullText();

    void setTextSpeed(float secondsPerChar) { m_textSpeed = secondsPerChar; }

    [[nodiscard]] const PostProcessSettings& getPostProcessSettings() const { return m_postProcessSettings; }
    void setPostProcessSettings(const PostProcessSettings& settings) { m_postProcessSettings = settings; }

    // Форматтер текста диалогов (например, автоматический перенос строк Word Wrapping)
    using TextFormatter = std::function<std::string(std::string_view)>;
    void setTextFormatter(TextFormatter formatter) { m_textFormatter = std::move(formatter); }

    // Управление переменными сценария
    void setVariable(const std::string& name, const ScriptValue& val);
    [[nodiscard]] ScriptValue getVariable(const std::string& name) const;
    [[nodiscard]] const std::unordered_map<std::string, ScriptValue>& getVariables() const { return m_variables; }
    void clearVariables();

    // Запуск и исполнение команд сценария
    void start() { executeNextCommand(); }
    void executeNextCommand();

private:
    void revealNextUtf8Char();
    [[nodiscard]] bool evaluateCondition(const std::string& condition) const;
    [[nodiscard]] ScriptValue evaluateExpression(const std::string& expr) const;
    [[nodiscard]] std::string interpolateVariables(std::string_view text) const;

private:
    TextFormatter m_textFormatter;
    AnimationEventHandler m_animHandler;
    TransitionEventHandler m_transHandler;
    AssetResolver m_assetResolver;
    ScriptChangeHandler m_scriptChangeHandler;
    AudioEventHandler m_audioHandler;

private:
    std::vector<ScriptCommand> m_commands;
    std::unordered_map<std::string, size_t> m_labelIndices;
    size_t m_currentCommandIndex = 0;

    // Переменные сценария
    std::unordered_map<std::string, ScriptValue> m_variables;

    std::string m_currentBackground;
    PostProcessSettings m_postProcessSettings;
    
    // Реестр персонажей и их активные слоты/слои
    std::unordered_map<std::string, CharacterDefinition> m_definedCharacters;
    std::unordered_map<std::string, ActiveCharacter> m_activeCharacters;

    std::string m_currentSpeaker;
    std::string m_fullDialogueText;
    std::string m_visibleDialogueText;
    size_t m_charByteIndex = 0;

    float m_textTimer = 0.0f;
    float m_textSpeed = 0.020f;
    float m_pauseTimer = 0.0f;

    bool m_isWaitingForChoice = false;
    std::vector<ChoiceOption> m_currentChoices;
    bool m_isFinished = false;
};

#endif //DARIVN_STORYENGINE_HPP
