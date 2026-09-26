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

#include "Button.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"

Button::Button(glm::vec2 position, glm::vec2 size, std::string text, std::function<void()> onClick)
    : m_position(position), m_size(size), m_text(std::move(text)), m_onClick(std::move(onClick)) {}

bool Button::update(glm::vec2 mousePos, bool mouseJustPressed) {
    m_hovered = isHovered(mousePos);
    if (m_hovered && mouseJustPressed && m_onClick) {
        if (s_clickCallback) s_clickCallback();
        m_onClick();
        return true;
    }
    return false;
}

void Button::render(SpriteRenderer& spriteRenderer, TextRenderer& textRenderer) {
    // 1. Цвет фона кнопки (подсвечивается при наведении мыши)
    glm::vec4 bgColor = m_hovered 
        ? glm::vec4(0.24f, 0.32f, 0.50f, 0.95f) 
        : glm::vec4(0.12f, 0.15f, 0.24f, 0.85f);
    spriteRenderer.drawRect(m_position, m_size, bgColor);

    // 2. Рамка кнопки
    glm::vec4 borderColor = m_hovered 
        ? glm::vec4(0.50f, 0.75f, 1.0f, 1.0f) 
        : glm::vec4(0.28f, 0.38f, 0.55f, 0.75f);
    
    float bw = 2.0f;
    spriteRenderer.drawRect(m_position, glm::vec2(m_size.x, bw), borderColor); // верх
    spriteRenderer.drawRect(glm::vec2(m_position.x, m_position.y + m_size.y - bw), glm::vec2(m_size.x, bw), borderColor); // низ
    spriteRenderer.drawRect(m_position, glm::vec2(bw, m_size.y), borderColor); // лево
    spriteRenderer.drawRect(glm::vec2(m_position.x + m_size.x - bw, m_position.y), glm::vec2(bw, m_size.y), borderColor); // право

    // 3. Выравнивание текста по центру кнопки с авто-подгонкой масштаба
    float scale = 0.85f;
    glm::vec2 textSize = textRenderer.measureText(m_text, scale);
    if (textSize.x > m_size.x - 24.0f && textSize.x > 0.0f) {
        scale *= (m_size.x - 24.0f) / textSize.x;
        textSize = textRenderer.measureText(m_text, scale);
    }
    float textX = m_position.x + (m_size.x - textSize.x) * 0.5f;
    float textY = m_position.y + (m_size.y - textSize.y) * 0.5f;

    glm::vec4 textColor = m_hovered 
        ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) 
        : glm::vec4(0.85f, 0.88f, 0.95f, 1.0f);

    textRenderer.renderText(m_text, textX, textY, scale, textColor);
}
