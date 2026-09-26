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

#include "PauseMenuState.hpp"
#include "MainMenuState.hpp"
#include "SaveLoadState.hpp"
#include "SettingsState.hpp"
#include "StateManager.hpp"

#include "core/Input.hpp"
#include "resource/Localization.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"
#include "audio/AudioEngine.hpp"

#include <GLFW/glfw3.h>

PauseMenuState::PauseMenuState(SharedContext& context)
    : IGameState(context) {
}

void PauseMenuState::onEnter() {
    if (m_context.audioEngine) {
        m_context.audioEngine->setMusicDucked(true);
    }
    initButtons();
}

void PauseMenuState::onExit() {
    if (m_context.audioEngine) {
        m_context.audioEngine->setMusicDucked(false);
    }
}

void PauseMenuState::onResume() {
    if (m_context.audioEngine) {
        m_context.audioEngine->setMusicDucked(true);
    }
    initButtons();
}

void PauseMenuState::initButtons() {
    m_buttons.clear();

    float cardX = 470.0f;
    float cardW = 340.0f;
    float btnW = 260.0f;
    float btnH = 42.0f;
    float btnX = cardX + (cardW - btnW) * 0.5f; // 510.0f
    float btnStartY = 236.0f;
    float spacing = 10.0f;

    // 1. Продолжить
    m_buttons.emplace_back(
        glm::vec2(btnX, btnStartY), glm::vec2(btnW, btnH),
        LOC("pause.resume"),
        [this]() {
            m_context.stateManager->popState();
        }
    );

    // 2. Сохранить
    m_buttons.emplace_back(
        glm::vec2(btnX, btnStartY + 1.0f * (btnH + spacing)), glm::vec2(btnW, btnH),
        LOC("pause.save"),
        [this]() {
            m_context.stateManager->pushState(std::make_unique<SaveLoadState>(m_context, true));
        }
    );

    // 3. Загрузить
    m_buttons.emplace_back(
        glm::vec2(btnX, btnStartY + 2.0f * (btnH + spacing)), glm::vec2(btnW, btnH),
        LOC("pause.load"),
        [this]() {
            m_context.stateManager->pushState(std::make_unique<SaveLoadState>(m_context, false));
        }
    );

    // 4. Настройки
    m_buttons.emplace_back(
        glm::vec2(btnX, btnStartY + 3.0f * (btnH + spacing)), glm::vec2(btnW, btnH),
        LOC("pause.settings"),
        [this]() {
            m_context.stateManager->pushState(std::make_unique<SettingsState>(m_context));
        }
    );

    // 5. Главное меню
    m_buttons.emplace_back(
        glm::vec2(btnX, btnStartY + 4.0f * (btnH + spacing)), glm::vec2(btnW, btnH),
        LOC("pause.main_menu"),
        [this]() {
            m_context.stateManager->changeState(std::make_unique<MainMenuState>(m_context));
        }
    );

    // 6. Выход
    m_buttons.emplace_back(
        glm::vec2(btnX, btnStartY + 5.0f * (btnH + spacing)), glm::vec2(btnW, btnH),
        LOC("pause.exit"),
        [this]() {
            if (m_context.quit) {
                m_context.quit();
            }
        }
    );
}

void PauseMenuState::handleInput() {
    // Закрытие паузы по Escape или ПКМ
    if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE) || Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        m_context.stateManager->popState();
        return;
    }

    glm::vec2 mousePos = Input::getMousePosition();
    bool mouseClicked = Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);

    for (auto& btn : m_buttons) {
        btn.update(mousePos, mouseClicked);
    }
}

void PauseMenuState::update(float dt) {
    (void)dt;
}

void PauseMenuState::render() {
    if (!m_context.spriteRenderer || !m_context.textRenderer) return;

    // Затемнение поверх размытия сцены
    m_context.spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.04f, 0.05f, 0.08f, 0.70f));

    // Карточка меню паузы
    float cardX = 470.0f;
    float cardY = 165.0f;
    float cardW = 340.0f;
    float cardH = 390.0f;

    m_context.spriteRenderer->drawRect(glm::vec2(cardX, cardY), glm::vec2(cardW, cardH), glm::vec4(0.08f, 0.09f, 0.14f, 0.94f));

    // Рамка карточки
    float bw = 2.0f;
    glm::vec4 borderCol(0.35f, 0.55f, 0.95f, 0.5f);
    m_context.spriteRenderer->drawRect(glm::vec2(cardX, cardY), glm::vec2(cardW, bw), borderCol);
    m_context.spriteRenderer->drawRect(glm::vec2(cardX, cardY + cardH - bw), glm::vec2(cardW, bw), borderCol);
    m_context.spriteRenderer->drawRect(glm::vec2(cardX, cardY), glm::vec2(bw, cardH), borderCol);
    m_context.spriteRenderer->drawRect(glm::vec2(cardX + cardW - bw, cardY), glm::vec2(bw, cardH), borderCol);

    // Заголовок карточки
    std::string titleStr = LOC("pause.title");
    float titleW = m_context.textRenderer->measureText(titleStr, 0.95f).x;
    m_context.textRenderer->renderText(titleStr, cardX + (cardW - titleW) * 0.5f, cardY + 18.0f, 0.95f, glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));

    std::string hintStr = LOC("pause.hint");
    float hintW = m_context.textRenderer->measureText(hintStr, 0.60f).x;
    m_context.textRenderer->renderText(hintStr, cardX + (cardW - hintW) * 0.5f, cardY + 45.0f, 0.60f, glm::vec4(0.55f, 0.60f, 0.70f, 0.85f));

    // Разделитель
    m_context.spriteRenderer->drawRect(glm::vec2(cardX + 24.0f, cardY + 65.0f), glm::vec2(cardW - 48.0f, 1.5f), glm::vec4(0.35f, 0.55f, 0.95f, 0.4f));

    // Отрисовка 6 кнопок
    for (auto& btn : m_buttons) {
        btn.render(*m_context.spriteRenderer, *m_context.textRenderer);
    }
}
