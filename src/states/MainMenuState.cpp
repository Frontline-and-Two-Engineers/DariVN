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

#include "MainMenuState.hpp"
#include "GameplayState.hpp"
#include "SaveLoadState.hpp"
#include "SettingsState.hpp"
#include "StateManager.hpp"

#include "core/Config.hpp"
#include "core/Input.hpp"
#include "resource/ResourceManager.hpp"
#include "resource/Localization.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"
#include "audio/AudioEngine.hpp"
#include "script/StoryEngine.hpp"
#include "scene/CharacterSprite.hpp"

MainMenuState::MainMenuState(SharedContext& context)
    : IGameState(context) {
}

void MainMenuState::onEnter() {
    if (m_context.audioEngine) {
        m_context.audioEngine->setMusicDucked(false);
        m_context.audioEngine->stopAmbient(1.0f);

        std::string menuMusic = Config::getString("menu.music", "assets/audio/bgm/menu_theme.mp3");
        float defaultBgmFade = Config::getFloat("audio.default_bgm_fade", 1.5f);
        if (!menuMusic.empty()) {
            std::string resolved = m_context.resolveAsset ? m_context.resolveAsset(menuMusic) : menuMusic;
            if (m_context.audioEngine->getCurrentMusicPath() != resolved || !m_context.audioEngine->isMusicPlaying()) {
                m_context.audioEngine->playMusic(resolved, true, defaultBgmFade);
            }
        }
    }
    initButtons();
}

void MainMenuState::onResume() {
    if (m_context.audioEngine) {
        m_context.audioEngine->setMusicDucked(false);
        m_context.audioEngine->stopAmbient(1.0f);

        std::string menuMusic = Config::getString("menu.music", "assets/audio/bgm/menu_theme.mp3");
        float defaultBgmFade = Config::getFloat("audio.default_bgm_fade", 1.5f);
        if (!menuMusic.empty()) {
            std::string resolved = m_context.resolveAsset ? m_context.resolveAsset(menuMusic) : menuMusic;
            if (m_context.audioEngine->getCurrentMusicPath() != resolved || !m_context.audioEngine->isMusicPlaying()) {
                m_context.audioEngine->playMusic(resolved, true, defaultBgmFade);
            }
        }
    }
    initButtons();
}

void MainMenuState::initButtons() {
    m_buttons.clear();

    m_buttons.emplace_back(
        glm::vec2(100.0f, 300.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.new_game"),
        [this]() {
            std::string startScript = Config::getString("start_script", "");
            if (startScript.empty()) {
                startScript = Config::getString("story.start_script", "assets/scripts/demo.vn");
            }
            if (m_context.characters) {
                m_context.characters->clear();
            }
            if (m_context.audioEngine) {
                m_context.audioEngine->setMusicDucked(false);
            }
            if (m_context.storyEngine) {
                std::string resolved = m_context.resolveAsset ? m_context.resolveAsset(startScript) : startScript;
                m_context.storyEngine->loadScript(resolved);
                m_context.storyEngine->executeNextCommand();
            }
            m_context.stateManager->changeState(std::make_unique<GameplayState>(m_context));
        }
    );

    m_buttons.emplace_back(
        glm::vec2(100.0f, 370.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.load"),
        [this]() {
            m_context.stateManager->pushState(std::make_unique<SaveLoadState>(m_context, false));
        }
    );

    m_buttons.emplace_back(
        glm::vec2(100.0f, 440.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.settings"),
        [this]() {
            m_context.stateManager->pushState(std::make_unique<SettingsState>(m_context));
        }
    );

    m_buttons.emplace_back(
        glm::vec2(100.0f, 510.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.exit"),
        [this]() {
            if (m_context.quit) {
                m_context.quit();
            }
        }
    );
}

void MainMenuState::handleInput() {
    glm::vec2 mousePos = Input::getMousePosition();
    bool mouseClicked = Input::isMouseButtonJustPressed(0);

    for (auto& btn : m_buttons) {
        btn.update(mousePos, mouseClicked);
    }
}

void MainMenuState::update(float dt) {
    (void)dt;
}

void MainMenuState::render() {
    if (!m_context.spriteRenderer || !m_context.textRenderer) return;

    // 1. Фоновое изображение главного меню
    std::string menuBgPath = Config::getString("menu.background", "assets/textures/backgrounds/classroom.jpg");
    std::string resolvedBg = m_context.resolveAsset ? m_context.resolveAsset(menuBgPath) : menuBgPath;
    auto menuBg = ResourceManager::loadTexture("menu_bg", resolvedBg);
    if (menuBg) {
        m_context.spriteRenderer->drawSprite(*menuBg, glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), 0.0f, glm::vec4(0.45f, 0.50f, 0.65f, 1.0f));
    }

    // 2. Декоративная боковая панель меню
    m_context.spriteRenderer->drawRect(glm::vec2(60.0f, 0.0f), glm::vec2(360.0f, 720.0f), glm::vec4(0.05f, 0.06f, 0.10f, 0.88f));
    m_context.spriteRenderer->drawRect(glm::vec2(420.0f, 0.0f), glm::vec2(3.0f, 720.0f), glm::vec4(0.35f, 0.55f, 0.95f, 0.6f));

    // 3. Название игры и подзаголовок из локализации
    m_context.textRenderer->renderText(LOC("menu.title"), 100.0f, 130.0f, 1.5f, glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));
    m_context.textRenderer->renderText(LOC("menu.subtitle"), 100.0f, 195.0f, 0.75f, glm::vec4(0.45f, 0.65f, 1.0f, 0.9f));

    // 4. Отрисовка кнопок меню
    for (auto& btn : m_buttons) {
        btn.render(*m_context.spriteRenderer, *m_context.textRenderer);
    }

    // 5. Версия из локализации в нижнем углу
    m_context.textRenderer->renderText(LOC("menu.version"), 950.0f, 680.0f, 0.65f, glm::vec4(0.4f, 0.45f, 0.55f, 0.8f));
}
