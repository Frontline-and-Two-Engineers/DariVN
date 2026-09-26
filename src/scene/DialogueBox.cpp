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
    // 3. Текст реплики диалога
    // -------------------------------------------------------------
    if (!text.empty()) {
        float textX = m_position.x + m_textPadding.x;
        float textY = m_position.y + m_textPadding.y;
        textRenderer.renderText(text, textX, textY, m_textScale, m_textColor);
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
