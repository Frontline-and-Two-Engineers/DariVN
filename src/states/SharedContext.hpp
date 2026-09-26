#pragma once

#ifndef DARIVN_SHAREDCONTEXT_HPP
#define DARIVN_SHAREDCONTEXT_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <functional>

class Window;
class SpriteRenderer;
class TextRenderer;
class RenderPipeline;
class AudioEngine;
class StoryEngine;
class DialogueBox;
class CharacterSprite;
class StateManager;

struct GameSaveState;

struct SharedContext {
    Window* window = nullptr;
    SpriteRenderer* spriteRenderer = nullptr;
    TextRenderer* textRenderer = nullptr;
    RenderPipeline* renderPipeline = nullptr;
    AudioEngine* audioEngine = nullptr;
    StoryEngine* storyEngine = nullptr;
    DialogueBox* dialogueBox = nullptr;
    std::unordered_map<std::string, std::unique_ptr<CharacterSprite>>* characters = nullptr;
    StateManager* stateManager = nullptr;

    // Хелперы и коллбэки приложения
    std::function<void(const std::string&)> showToast;
    std::function<void(std::vector<uint8_t>&, int&, int&)> captureScreenThumbnail;
    std::function<void()> quit;
    std::function<std::string(const std::string&)> resolveAsset;
    std::function<CharacterSprite&(const std::string&)> getOrCreateCharacter;
    std::function<void()> applyAudioSettings;
    std::function<void()> applySettings;
    std::function<bool(const GameSaveState&)> restoreGameState;
};

#endif // DARIVN_SHAREDCONTEXT_HPP
