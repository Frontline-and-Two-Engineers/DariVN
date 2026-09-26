#pragma once

#ifndef DARIVN_APPLICATION_HPP
#define DARIVN_APPLICATION_HPP

#include <memory>
#include <vector>
#include "scene/Button.hpp"
#include "scene/CharacterSprite.hpp"
#include "scene/DialogueBox.hpp"
#include "script/StoryEngine.hpp"
#include "core/SaveManager.hpp"

class Window;
class RenderPipeline;
class Scene;
class PythonEngine;
class SpriteRenderer;
class TextRenderer;
class AudioEngine;

enum class GameState {
    MainMenu,
    Gameplay,
    PauseMenu,
    Backlog,
    SaveMenu,
    LoadMenu,
    SettingsMenu
};

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();

private:
    bool init();
    void processInput(float dt);
    void update(float dt);
    void render();
    void shutdown();

    void initUI();
    void updateChoiceButtons();
    void renderMainMenu();
    void renderGameplay();
    void renderPauseMenu();
    void renderBacklog();
    void renderSaveLoadMenu(bool isSaving);
    void renderSettingsMenu();

    void openSaveMenu();
    void openLoadMenu(GameState returnState);
    void openSettingsMenu(GameState returnState);
    void applySettings();
    void applyAudioSettings();
    void performSave(int slotIndex);
    void performLoad(int slotIndex);
    void refreshSaveSlotsMetadata();
    void showToast(const std::string& message);
    void captureScreenThumbnail(std::vector<uint8_t>& outPixels, int& outW, int& outH);

    CharacterSprite& getOrCreateCharacter(const std::string& name);

private:
    bool m_isRunning = false;
    float m_lastFrameTime = 0.0f;

    GameState m_gameState = GameState::MainMenu;

    std::unique_ptr<Window> m_window;
    std::unique_ptr<SpriteRenderer> m_spriteRenderer;
    std::unique_ptr<TextRenderer> m_textRenderer;
    std::unique_ptr<StoryEngine> m_storyEngine;
    std::unique_ptr<DialogueBox> m_dialogueBox;

    std::unordered_map<std::string, std::unique_ptr<CharacterSprite>> m_characters;

    std::vector<Button> m_mainMenuButtons;
    std::vector<Button> m_pauseMenuButtons;
    std::unique_ptr<Button> m_backlogCloseButton;
    std::vector<Button> m_choiceButtons;
    glm::vec2 m_choiceWindowPos{290.0f, 160.0f};
    glm::vec2 m_choiceWindowSize{700.0f, 260.0f};

    // Состояние беклога (скроллинг и интерфейс)
    float m_backlogScrollY = 0.0f;
    float m_backlogMaxScrollY = 0.0f;
    bool m_isDraggingScrollbar = false;
    float m_scrollbarDragStartMouseY = 0.0f;
    float m_scrollbarDragStartScrollY = 0.0f;

    std::unique_ptr<RenderPipeline> m_renderPipeline;
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::string m_clickSoundPath;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<PythonEngine> m_pythonEngine;

    // Сохранение и загрузка
    GameState m_saveLoadReturnState = GameState::PauseMenu;
    std::unique_ptr<Button> m_saveLoadBackButton;
    std::vector<SaveSlotMetadata> m_cachedSlots;
    SaveSlotMetadata m_cachedQuickSave;
    std::string m_toastMessage;
    float m_toastTimer = 0.0f;

    // Меню настроек
    GameState m_settingsReturnState = GameState::MainMenu;
    std::unique_ptr<Button> m_settingsBackButton;
    std::unique_ptr<Button> m_settingsResetButton;
    int m_activeDraggingSlider = -1; // -1: none, 0..4: audio, 5: text speed
};

#endif //DARIVN_APPLICATION_HPP
