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
