#pragma once

#ifndef DARIVN_APPLICATION_HPP
#define DARIVN_APPLICATION_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

class Window;
class SpriteRenderer;
class TextRenderer;
class StoryEngine;
class DialogueBox;
class CharacterSprite;
class RenderPipeline;
class AudioEngine;
class PythonEngine;
class StateManager;
struct SharedContext;
struct GameSaveState;

class Application {
public:
    Application();
    ~Application();

    bool init();
    void run();
    void shutdown();

    CharacterSprite& getOrCreateCharacter(const std::string& name);
    void showToast(const std::string& message);
    void captureScreenThumbnail(std::vector<uint8_t>& outPixels, int& outW, int& outH);
    bool restoreGameState(const GameSaveState& state);
    void applyAudioSettings();
    void applySettings();

private:
    void processInput();
    void update(float dt);
    void render();

    bool m_isRunning = false;
    float m_lastFrameTime = 0.0f;

    std::unique_ptr<Window> m_window;
    std::unique_ptr<SpriteRenderer> m_spriteRenderer;
    std::unique_ptr<TextRenderer> m_textRenderer;
    std::unique_ptr<StoryEngine> m_storyEngine;
    std::unique_ptr<DialogueBox> m_dialogueBox;
    std::unordered_map<std::string, std::unique_ptr<CharacterSprite>> m_characters;

    std::unique_ptr<RenderPipeline> m_renderPipeline;
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::string m_clickSoundPath;
    std::unique_ptr<PythonEngine> m_pythonEngine;

    std::unique_ptr<StateManager> m_stateManager;
    std::unique_ptr<SharedContext> m_context;

    std::string m_toastMessage;
    float m_toastTimer = 0.0f;
};

#endif // DARIVN_APPLICATION_HPP
