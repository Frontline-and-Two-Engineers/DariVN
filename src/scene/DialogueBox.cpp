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

#include "DialogueBox.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"
#include "renderer/Texture2D.hpp"
#include "resource/ResourceManager.hpp"

#include <glad/glad.h>
#include <cmath>
#include <algorithm>

DialogueBox::DialogueBox(glm::vec2 position, glm::vec2 size)
    : m_position(position), m_size(size) {}

void DialogueBox::setSkinTexture(const std::string& texturePath) {
    m_skinTexture = ResourceManager::loadTexture("dialogue_box_skin", texturePath);
}

void DialogueBox::setNameplateSkinTexture(const std::string& texturePath) {
    m_nameplateSkinTexture = ResourceManager::loadTexture("nameplate_skin", texturePath);
}

void DialogueBox::update(float deltaTime) {
    m_blinkTimer += deltaTime;
    m_bounceOffset = std::sin(m_blinkTimer * 5.0f) * 3.0f;
}

void DialogueBox::render(SpriteRenderer& spriteRenderer, TextRenderer& textRenderer,
                        const std::string& speaker,
                        const std::string& text,
                        bool isWaitingForClick) {
    // -------------------------------------------------------------
    // 1. Отрисовка основного диалогового окна
    // -------------------------------------------------------------
    if (m_skinTexture && m_skinTexture->isValid()) {
        spriteRenderer.drawSprite(*m_skinTexture, m_position, m_size);
    } else {
        // Процедурный фон окна
        spriteRenderer.drawRect(m_position, m_size, m_backgroundColor);
        // Верхняя акцентная граница
        spriteRenderer.drawRect(m_position, glm::vec2(m_size.x, 3.0f), m_borderColor);
        // Тонкая нижняя рамка
        spriteRenderer.drawRect(m_position + glm::vec2(0.0f, m_size.y - 1.0f), glm::vec2(m_size.x, 1.0f), m_borderColor * 0.5f);
    }

    // -------------------------------------------------------------
    // 2. Плашка имени говорящего (Nameplate)
    // -------------------------------------------------------------
    if (!speaker.empty()) {
        float scale = m_nameplateScale;
        glm::vec2 nameSize = textRenderer.measureText(speaker, scale);

        float padX = 24.0f;
        float padY = 8.0f;
        float plateH = std::max(44.0f, nameSize.y + padY * 2.0f);
        float plateW = std::max(200.0f, nameSize.x + padX * 2.0f);
        glm::vec2 platePos(m_position.x + 20.0f, m_position.y - plateH);

        if (m_nameplateSkinTexture && m_nameplateSkinTexture->isValid()) {
            spriteRenderer.drawSprite(*m_nameplateSkinTexture, platePos, glm::vec2(plateW, plateH));
        } else {
            // Фон плашки имени
            spriteRenderer.drawRect(platePos, glm::vec2(plateW, plateH), m_nameplateBgColor);
            // Верхняя акцентная полоска
            spriteRenderer.drawRect(platePos, glm::vec2(plateW, 2.0f), m_borderColor);
            // Нижняя линия разделения с окном диалога
            spriteRenderer.drawRect(platePos + glm::vec2(0.0f, plateH - 2.0f), glm::vec2(plateW, 2.0f), m_accentColor);
        }

        // Отрисовка текста имени с идеальным вертикальным центрированием
        float nameX = platePos.x + padX;
        float nameY = platePos.y + (plateH - nameSize.y) * 0.5f;
        textRenderer.renderText(speaker, nameX, nameY, scale, m_nameplateTextColor);
    }

    // -------------------------------------------------------------
    // -------------------------------------------------------------
    // 3. Текст реплики диалога
    // -------------------------------------------------------------
    if (!text.empty()) {
        float clipX = m_position.x + 8.0f;
        float clipY = m_position.y + 4.0f;
        float clipW = m_size.x - 16.0f;
        float clipH = m_size.y - 8.0f;

        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
        float scaleX = static_cast<float>(vp[2]) / 1280.0f;
        float scaleY = static_cast<float>(vp[3]) / 720.0f;

        GLint scissorX = static_cast<GLint>(clipX * scaleX + static_cast<float>(vp[0]));
        GLint scissorY = static_cast<GLint>((720.0f - (clipY + clipH)) * scaleY + static_cast<float>(vp[1]));
        GLint scissorW = static_cast<GLint>(clipW * scaleX);
        GLint scissorH = static_cast<GLint>(clipH * scaleY);

        GLboolean wasScissor = glIsEnabled(GL_SCISSOR_TEST);
        GLint prevScissor[4];
        glGetIntegerv(GL_SCISSOR_BOX, prevScissor);

        glEnable(GL_SCISSOR_TEST);
        glScissor(scissorX, scissorY, scissorW, scissorH);

        float textX = m_position.x + m_textPadding.x;
        float textY = m_position.y + m_textPadding.y;
        textRenderer.renderText(text, textX, textY, m_textScale, m_textColor);

        if (!wasScissor) {
            glDisable(GL_SCISSOR_TEST);
        } else {
            glScissor(prevScissor[0], prevScissor[1], prevScissor[2], prevScissor[3]);
        }
    }

    // -------------------------------------------------------------
    // 4. Анимированный индикатор завершения фразы (готовности перехода)
    // -------------------------------------------------------------
    if (isWaitingForClick) {
        float alpha = 0.45f + 0.55f * std::max(0.0f, std::sin(m_blinkTimer * 5.0f));
        float markerW = 14.0f;
        float markerH = 14.0f;
        float markerX = m_position.x + m_size.x - m_textPadding.x;
        float markerY = m_position.y + m_size.y - 32.0f + m_bounceOffset;

        // Стильный пульсирующий шеврон-маркер
        spriteRenderer.drawRect(
            glm::vec2(markerX, markerY), 
            glm::vec2(markerW, markerH), 
            glm::vec4(m_accentColor.r, m_accentColor.g, m_accentColor.b, alpha)
        );
        // Внутренний блик
        spriteRenderer.drawRect(
            glm::vec2(markerX + 3.0f, markerY + 3.0f), 
            glm::vec2(markerW - 6.0f, markerH - 6.0f), 
            glm::vec4(1.0f, 1.0f, 1.0f, alpha * 0.9f)
        );
    }
}

void DialogueBox::renderNVL(SpriteRenderer& spriteRenderer, TextRenderer& textRenderer,
                            const std::vector<NVLLine>& nvlLines,
                            const std::string& currentSpeaker,
                            const std::string& currentText,
                            bool isWaitingForClick) {
    // 1. Полноэкранное атмосферное затемнение сцены (в стиле Бесконечного Лета)
    spriteRenderer.drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.01f, 0.02f, 0.04f, 0.52f));

    // 2. Полупрозрачная панель страницы новеллы
    float boxX = 70.0f;
    float boxY = 28.0f;
    float boxW = 1140.0f;
    float boxH = 628.0f;

    // Фон пергамента/панели
    spriteRenderer.drawRect(glm::vec2(boxX, boxY), glm::vec2(boxW, boxH), glm::vec4(0.04f, 0.05f, 0.08f, 0.92f));

    // Тонкие рамки окна
    glm::vec4 frameColor(0.35f, 0.55f, 0.85f, 0.65f);
    glm::vec4 frameSideColor(0.20f, 0.30f, 0.50f, 0.35f);
    spriteRenderer.drawRect(glm::vec2(boxX, boxY), glm::vec2(boxW, 2.0f), frameColor); // верх
    spriteRenderer.drawRect(glm::vec2(boxX, boxY + boxH - 2.0f), glm::vec2(boxW, 2.0f), frameColor * 0.7f); // низ
    spriteRenderer.drawRect(glm::vec2(boxX, boxY), glm::vec2(1.5f, boxH), frameSideColor); // лево
    spriteRenderer.drawRect(glm::vec2(boxX + boxW - 1.5f, boxY), glm::vec2(1.5f, boxH), frameSideColor); // право

    // Золотой акцентный штрих сверху по центру
    float accentW = 240.0f;
    spriteRenderer.drawRect(
        glm::vec2(boxX + (boxW - accentW) * 0.5f, boxY), 
        glm::vec2(accentW, 2.5f), 
        glm::vec4(0.95f, 0.75f, 0.30f, 0.90f)
    );

    // 3. Вывод текста реплик страницы с аппаратным отсечением (Scissor Box)
    float padX = 55.0f;
    float padY = 40.0f;
    float textScale = 0.78f;
    float speakerScale = 0.80f;
    float lineHeight = textRenderer.getLineHeight() * textScale + 5.0f;
    float paragraphSpacing = 16.0f;

    // Подсчет общей высоты для плавного автоскролла при заполнении страницы
    float totalH = 0.0f;
    for (const auto& line : nvlLines) {
        if (!line.speaker.empty()) {
            totalH += lineHeight * 0.95f;
        }
        int lc = 1;
        for (char c : line.text) if (c == '\n') lc++;
        totalH += static_cast<float>(lc) * lineHeight + paragraphSpacing;
    }
    if (!currentSpeaker.empty()) {
        totalH += lineHeight * 0.95f;
    }
    if (!currentText.empty()) {
        int lc = 1;
        for (char c : currentText) if (c == '\n') lc++;
        totalH += static_cast<float>(lc) * lineHeight;
    }

    float maxVisibleH = boxH - padY * 2.0f - 20.0f;
    float scrollOffset = (totalH > maxVisibleH) ? (totalH - maxVisibleH) : 0.0f;

    float currentY = boxY + padY - scrollOffset;

    // Аппаратное ограничение области рендеринга текста через OpenGL Scissor Box:
    // Текст гарантированно не вылезет за пределы чатбокса сверху, снизу или по бокам
    float clipX = boxX + 8.0f;
    float clipY = boxY + 12.0f;
    float clipW = boxW - 16.0f;
    float clipH = boxH - 24.0f;

    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float scaleX = static_cast<float>(vp[2]) / 1280.0f;
    float scaleY = static_cast<float>(vp[3]) / 720.0f;

    GLint scissorX = static_cast<GLint>(clipX * scaleX + static_cast<float>(vp[0]));
    GLint scissorY = static_cast<GLint>((720.0f - (clipY + clipH)) * scaleY + static_cast<float>(vp[1]));
    GLint scissorW = static_cast<GLint>(clipW * scaleX);
    GLint scissorH = static_cast<GLint>(clipH * scaleY);

    GLboolean wasScissor = glIsEnabled(GL_SCISSOR_TEST);
    GLint prevScissor[4];
    glGetIntegerv(GL_SCISSOR_BOX, prevScissor);

    glEnable(GL_SCISSOR_TEST);
    glScissor(scissorX, scissorY, scissorW, scissorH);

    // Рендерим предыдущие прочитанные реплики страницы
    for (const auto& line : nvlLines) {
        int lc = 1;
        for (char c : line.text) if (c == '\n') lc++;
        float speakerH = !line.speaker.empty() ? (lineHeight * 0.95f) : 0.0f;
        float textH = static_cast<float>(lc) * lineHeight;
        float blockH = speakerH + textH + paragraphSpacing;

        // Отрисовываем реплику, если она пересекает видимую область панели
        if (currentY + blockH >= clipY && currentY <= clipY + clipH) {
            float textY = currentY;
            if (!line.speaker.empty()) {
                textRenderer.renderText(line.speaker, boxX + padX, textY, speakerScale, glm::vec4(1.0f, 0.85f, 0.45f, 1.0f));
                textY += speakerH;
            }
            textRenderer.renderText(line.text, boxX + padX, textY, textScale, glm::vec4(0.88f, 0.90f, 0.95f, 0.90f));
        }

        currentY += blockH;
    }

    // Рендерим текущую активную реплику (печатающуюся прямо сейчас)
    float lastLineEndY = currentY;
    if (!currentSpeaker.empty()) {
        float speakerH = lineHeight * 0.95f;
        if (currentY + speakerH >= clipY && currentY <= clipY + clipH) {
            textRenderer.renderText(currentSpeaker, boxX + padX, currentY, speakerScale, glm::vec4(1.0f, 0.85f, 0.45f, 1.0f));
        }
        currentY += speakerH;
    }

    if (!currentText.empty()) {
        int lc = 1;
        for (char c : currentText) if (c == '\n') lc++;
        float textH = static_cast<float>(lc) * lineHeight;
        if (currentY + textH >= clipY && currentY <= clipY + clipH) {
            textRenderer.renderText(currentText, boxX + padX, currentY, textScale, glm::vec4(0.96f, 0.98f, 1.0f, 1.0f));
        }
        lastLineEndY = currentY + textH;
    }

    // 4. Индикатор завершения текущей реплики (ожидание клика игрока)
    if (isWaitingForClick) {
        float alpha = 0.45f + 0.55f * std::max(0.0f, std::sin(m_blinkTimer * 5.0f));
        float markerW = 14.0f;
        float markerH = 14.0f;
        float markerX = boxX + boxW - padX;
        float markerY = std::min(boxY + boxH - padY - 10.0f, lastLineEndY + 6.0f) + m_bounceOffset;

        spriteRenderer.drawRect(
            glm::vec2(markerX, markerY), 
            glm::vec2(markerW, markerH), 
            glm::vec4(0.95f, 0.75f, 0.30f, alpha)
        );
        spriteRenderer.drawRect(
            glm::vec2(markerX + 3.0f, markerY + 3.0f), 
            glm::vec2(markerW - 6.0f, markerH - 6.0f), 
            glm::vec4(1.0f, 1.0f, 1.0f, alpha * 0.9f)
        );
    }

    // Восстанавливаем состояние scissor
    if (!wasScissor) {
        glDisable(GL_SCISSOR_TEST);
    } else {
        glScissor(prevScissor[0], prevScissor[1], prevScissor[2], prevScissor[3]);
    }
}
