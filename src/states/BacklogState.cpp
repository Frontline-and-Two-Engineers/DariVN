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

#include "BacklogState.hpp"
#include "StateManager.hpp"

#include "core/Window.hpp"
#include "core/Input.hpp"
#include "resource/Localization.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"
#include "audio/AudioEngine.hpp"
#include "script/StoryEngine.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

BacklogState::BacklogState(SharedContext& context)
    : IGameState(context) {
}

void BacklogState::onEnter() {
    if (m_context.audioEngine) {
        m_context.audioEngine->setMusicDucked(true);
    }

    m_closeButton = std::make_unique<Button>(
        glm::vec2(1080.0f, 18.0f), glm::vec2(120.0f, 44.0f),
        LOC("backlog.close"),
        [this]() {
            m_context.stateManager->popState();
        }
    );

    // Вычисляем высоту элементов и перематываем в самый конец
    if (m_context.storyEngine && m_context.textRenderer) {
        const auto& history = m_context.storyEngine->getDialogueHistory();
        float boxW = 1060.0f;
        float boxH = 618.0f;
        float itemSpacing = 16.0f;
        float textScale = 0.68f;

        float totalHeight = 0.0f;
        for (const auto& entry : history) {
            std::string wrapped = m_context.textRenderer->wrapText(entry.text, boxW - 36.0f, textScale);
            int lineCount = 1;
            for (char c : wrapped) {
                if (c == '\n') lineCount++;
            }
            float linesH = static_cast<float>(lineCount) * (m_context.textRenderer->getLineHeight() * textScale + 4.0f);
            float itemH = entry.speaker.empty() ? (linesH + 24.0f) : (linesH + 54.0f);
            totalHeight += itemH + itemSpacing;
        }
        if (totalHeight > 0.0f) totalHeight -= itemSpacing;

        m_maxScrollY = std::max(0.0f, totalHeight - boxH);
        m_scrollY = m_maxScrollY;
    }
}

void BacklogState::onExit() {
    if (m_context.audioEngine) {
        m_context.audioEngine->setMusicDucked(false);
    }
}

void BacklogState::handleInput() {
    // 1. Закрытие по Escape, ПКМ или клавише B
    if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE) ||
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT) ||
        Input::isKeyJustPressed(GLFW_KEY_B)) {
        m_context.stateManager->popState();
        return;
    }

    glm::vec2 mousePos = Input::getMousePosition();
    bool mouseClicked = Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);

    // 2. Кнопка "Закрыть"
    if (m_closeButton) {
        m_closeButton->update(mousePos, mouseClicked);
    }

    // 3. Прокрутка колесом мыши
    float scrollDelta = Input::getMouseScrollY();
    if (std::abs(scrollDelta) > 0.01f) {
        if (scrollDelta < -0.1f && m_scrollY >= m_maxScrollY - 0.5f) {
            // Прокрутка вниз в самом конце закрывает бэклог
            m_context.stateManager->popState();
            return;
        }
        m_scrollY -= scrollDelta * 60.0f;
    }

    // 4. Навигация с клавиатуры
    if (Input::isKeyJustPressed(GLFW_KEY_UP)) {
        m_scrollY -= 60.0f;
    }
    if (Input::isKeyJustPressed(GLFW_KEY_DOWN)) {
        if (m_scrollY >= m_maxScrollY - 0.5f) {
            m_context.stateManager->popState();
            return;
        }
        m_scrollY += 60.0f;
    }
    if (Input::isKeyJustPressed(GLFW_KEY_PAGE_UP)) {
        m_scrollY -= 300.0f;
    }
    if (Input::isKeyJustPressed(GLFW_KEY_PAGE_DOWN)) {
        m_scrollY += 300.0f;
    }
    if (Input::isKeyJustPressed(GLFW_KEY_HOME)) {
        m_scrollY = 0.0f;
    }
    if (Input::isKeyJustPressed(GLFW_KEY_END)) {
        m_scrollY = m_maxScrollY;
    }

    // 5. Интерактивный скроллбар
    float trackX = 1155.0f;
    float trackY = 78.0f;
    float trackW = 12.0f;
    float trackH = 618.0f;

    if (Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (mousePos.x >= trackX - 8.0f && mousePos.x <= trackX + trackW + 8.0f &&
            mousePos.y >= trackY && mousePos.y <= trackY + trackH) {
            m_isDraggingScrollbar = true;
            if (trackH > 40.0f && m_maxScrollY > 0.0f) {
                float thumbH = std::max(40.0f, trackH * (trackH / (trackH + m_maxScrollY)));
                float travel = trackH - thumbH;
                if (travel > 1.0f) {
                    float targetY = std::clamp(mousePos.y - trackY - thumbH * 0.5f, 0.0f, travel);
                    m_scrollY = (targetY / travel) * m_maxScrollY;
                }
            }
            m_scrollbarDragStartMouseY = mousePos.y;
            m_scrollbarDragStartScrollY = m_scrollY;
        }
    }
    if (Input::isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
        m_isDraggingScrollbar = false;
    }
    if (m_isDraggingScrollbar) {
        float deltaMouseY = mousePos.y - m_scrollbarDragStartMouseY;
        if (trackH > 40.0f && m_maxScrollY > 0.0f) {
            float thumbH = std::max(40.0f, trackH * (trackH / (trackH + m_maxScrollY)));
            float travel = trackH - thumbH;
            if (travel > 1.0f) {
                m_scrollY = m_scrollbarDragStartScrollY + (deltaMouseY / travel) * m_maxScrollY;
            }
        }
    }

    m_scrollY = std::clamp(m_scrollY, 0.0f, std::max(0.0f, m_maxScrollY));
}

void BacklogState::update(float dt) {
    (void)dt;
}

void BacklogState::render() {
    if (!m_context.spriteRenderer || !m_context.textRenderer || !m_context.storyEngine) return;

    // 1. Затемнение фона поверх размытия сцены
    m_context.spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.04f, 0.05f, 0.08f, 0.94f));

    // 2. Верхняя панель заголовка
    m_context.textRenderer->renderText(LOC("backlog.title"), 80.0f, 14.0f, 0.95f, glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));
    m_context.textRenderer->renderText(LOC("backlog.hint"), 80.0f, 44.0f, 0.58f, glm::vec4(0.55f, 0.60f, 0.70f, 0.85f));

    if (m_closeButton) {
        m_closeButton->render(*m_context.spriteRenderer, *m_context.textRenderer);
    }

    // Декоративная разделительная полоса
    m_context.spriteRenderer->drawRect(glm::vec2(80.0f, 70.0f), glm::vec2(1087.0f, 2.0f), glm::vec4(0.35f, 0.55f, 0.95f, 0.5f));

    // 3. Вычисление размеров реплик
    const auto& history = m_context.storyEngine->getDialogueHistory();
    float boxX = 80.0f;
    float boxY = 78.0f;
    float boxW = 1060.0f;
    float boxH = 618.0f;

    struct BacklogLayoutItem {
        std::string wrappedText;
        float height;
    };
    std::vector<BacklogLayoutItem> items;
    items.reserve(history.size());

    float totalHeight = 0.0f;
    float itemSpacing = 16.0f;
    float textScale = 0.68f;
    float speakerScale = 0.80f;

    for (const auto& entry : history) {
        std::string wrapped = m_context.textRenderer->wrapText(entry.text, boxW - 36.0f, textScale);
        int lineCount = 1;
        for (char c : wrapped) {
            if (c == '\n') lineCount++;
        }
        float linesH = static_cast<float>(lineCount) * (m_context.textRenderer->getLineHeight() * textScale + 4.0f);
        float itemH = entry.speaker.empty() ? (linesH + 24.0f) : (linesH + 54.0f);
        items.push_back({wrapped, itemH});
        totalHeight += itemH + itemSpacing;
    }
    if (totalHeight > 0.0f) {
        totalHeight -= itemSpacing;
    }

    m_maxScrollY = std::max(0.0f, totalHeight - boxH);
    m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScrollY);

    // 4. Ограничение области отрисовки через OpenGL Scissor Box (с учетом Retina/HiDPI)
    int fbWidth = 1280, fbHeight = 720;
    if (m_context.window) {
        m_context.window->getFramebufferSize(&fbWidth, &fbHeight);
    }
    float scaleX = static_cast<float>(fbWidth) / 1280.0f;
    float scaleY = static_cast<float>(fbHeight) / 720.0f;

    GLint scissorX = static_cast<GLint>(boxX * scaleX);
    GLint scissorY = static_cast<GLint>((720.0f - (boxY + boxH)) * scaleY);
    GLint scissorW = static_cast<GLint>(boxW * scaleX);
    GLint scissorH = static_cast<GLint>(boxH * scaleY);

    glEnable(GL_SCISSOR_TEST);
    glScissor(scissorX, scissorY, scissorW, scissorH);

    float currentY = boxY + 10.0f - m_scrollY;
    for (size_t i = 0; i < history.size(); ++i) {
        const auto& entry = history[i];
        float itemH = items[i].height;

        if (currentY + itemH >= boxY && currentY <= boxY + boxH) {
            m_context.spriteRenderer->drawRect(glm::vec2(boxX, currentY), glm::vec2(boxW, itemH), glm::vec4(0.07f, 0.09f, 0.14f, 0.82f));

            glm::vec4 accentColor = entry.speaker.empty() ? glm::vec4(0.40f, 0.50f, 0.65f, 0.7f) : glm::vec4(0.95f, 0.58f, 0.25f, 0.95f);
            m_context.spriteRenderer->drawRect(glm::vec2(boxX, currentY), glm::vec2(3.5f, itemH), accentColor);
            m_context.spriteRenderer->drawRect(glm::vec2(boxX, currentY + itemH - 1.0f), glm::vec2(boxW, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.05f));

            float textStartY = currentY + 12.0f;
            if (!entry.speaker.empty()) {
                m_context.textRenderer->renderText(entry.speaker, boxX + 18.0f, currentY + 10.0f, speakerScale, glm::vec4(1.0f, 0.85f, 0.40f, 1.0f));
                textStartY = currentY + 40.0f;
            }

            m_context.textRenderer->renderText(items[i].wrappedText, boxX + 18.0f, textStartY, textScale, glm::vec4(0.92f, 0.94f, 0.98f, 1.0f));
        }

        currentY += itemH + itemSpacing;
    }

    glDisable(GL_SCISSOR_TEST);

    // 5. Отрисовка скроллбара
    float trackX = 1155.0f;
    float trackY = 78.0f;
    float trackW = 12.0f;
    float trackH = 618.0f;

    m_context.spriteRenderer->drawRect(glm::vec2(trackX, trackY), glm::vec2(trackW, trackH), glm::vec4(0.08f, 0.10f, 0.16f, 0.60f));

    if (m_maxScrollY > 0.0f) {
        float thumbH = std::max(40.0f, trackH * (trackH / (trackH + m_maxScrollY)));
        float scrollRatio = m_scrollY / m_maxScrollY;
        float thumbY = trackY + scrollRatio * (trackH - thumbH);
        glm::vec4 thumbColor = m_isDraggingScrollbar ? glm::vec4(0.60f, 0.80f, 1.0f, 0.95f) : glm::vec4(0.35f, 0.55f, 0.95f, 0.80f);
        m_context.spriteRenderer->drawRect(glm::vec2(trackX, thumbY), glm::vec2(trackW, thumbH), thumbColor);
    } else {
        m_context.spriteRenderer->drawRect(glm::vec2(trackX, trackY), glm::vec2(trackW, trackH), glm::vec4(0.20f, 0.25f, 0.35f, 0.30f));
    }
}
