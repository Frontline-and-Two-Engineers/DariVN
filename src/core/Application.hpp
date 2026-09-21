#pragma once

#ifndef DARIVN_APPLICATION_HPP
#define DARIVN_APPLICATION_HPP

#include <memory>
#include <vector>
#include "scene/Button.hpp"
#include "scene/CharacterSprite.hpp"
#include "scene/DialogueBox.hpp"
#include "script/StoryEngine.hpp"

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
    PauseMenu
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
    std::vector<Button> m_choiceButtons;
    glm::vec2 m_choiceWindowPos{290.0f, 160.0f};
    glm::vec2 m_choiceWindowSize{700.0f, 260.0f};

    std::unique_ptr<RenderPipeline> m_renderPipeline;
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::string m_clickSoundPath;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<PythonEngine> m_pythonEngine;
};

#endif //DARIVN_APPLICATION_HPP
