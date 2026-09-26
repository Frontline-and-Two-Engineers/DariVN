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

#ifndef DARIVN_DIALOGUEBOX_HPP
#define DARIVN_DIALOGUEBOX_HPP

#include <string>
#include <memory>
#include <glm/glm.hpp>

class SpriteRenderer;
class TextRenderer;
class Texture2D;

class DialogueBox {
public:
    DialogueBox(glm::vec2 position = glm::vec2(40.0f, 490.0f),
                glm::vec2 size = glm::vec2(1200.0f, 195.0f));
    ~DialogueBox() = default;

    // Обновление внутренних анимаций (мигание маркера готовности перехода)
    void update(float deltaTime);

    // Отрисовка диалогового окна, плашки имени, текста реплики и маркера перехода
    void render(SpriteRenderer& spriteRenderer, TextRenderer& textRenderer,
                const std::string& speaker,
                const std::string& text,
                bool isWaitingForClick);

    // Установка скина/текстуры диалогового окна
    void setSkinTexture(std::shared_ptr<Texture2D> texture) { m_skinTexture = std::move(texture); }
    void setSkinTexture(const std::string& texturePath);

    // Установка скина/текстуры плашки имени
    void setNameplateSkinTexture(std::shared_ptr<Texture2D> texture) { m_nameplateSkinTexture = std::move(texture); }
    void setNameplateSkinTexture(const std::string& texturePath);

    // Позиция и размеры
    void setPosition(glm::vec2 pos) { m_position = pos; }
    [[nodiscard]] glm::vec2 getPosition() const { return m_position; }

    void setSize(glm::vec2 size) { m_size = size; }
    [[nodiscard]] glm::vec2 getSize() const { return m_size; }

    // Отступы текста внутри диалогового окна
    void setTextPadding(glm::vec2 padding) { m_textPadding = padding; }
    [[nodiscard]] glm::vec2 getTextPadding() const { return m_textPadding; }

    // Доступная максимальная ширина для переноса текста
    [[nodiscard]] float getMaxTextWidth() const { return m_size.x - m_textPadding.x * 2.0f; }
    [[nodiscard]] float getTextScale() const { return m_textScale; }
    void setTextScale(float scale) { m_textScale = scale; }

    [[nodiscard]] float getNameplateScale() const { return m_nameplateScale; }
    void setNameplateScale(float scale) { m_nameplateScale = scale; }

    // Кастомизация цветов процедурного стиля
    void setBackgroundColor(glm::vec4 color) { m_backgroundColor = color; }
    void setBorderColor(glm::vec4 color) { m_borderColor = color; }
    void setAccentColor(glm::vec4 color) { m_accentColor = color; }
    void setTextColor(glm::vec4 color) { m_textColor = color; }
    void setNameplateTextColor(glm::vec4 color) { m_nameplateTextColor = color; }
    void setNameplateBgColor(glm::vec4 color) { m_nameplateBgColor = color; }

private:
    glm::vec2 m_position;
    glm::vec2 m_size;
    glm::vec2 m_textPadding{45.0f, 38.0f};

    // Цвета процедурного стиля окна
    glm::vec4 m_backgroundColor{0.04f, 0.05f, 0.09f, 0.88f};
    glm::vec4 m_borderColor{0.35f, 0.55f, 0.95f, 0.95f};
    glm::vec4 m_accentColor{0.95f, 0.55f, 0.25f, 1.0f};
    glm::vec4 m_textColor{0.94f, 0.96f, 1.0f, 1.0f};

    // Цвета плашки имени
    glm::vec4 m_nameplateBgColor{0.14f, 0.17f, 0.26f, 0.95f};
    glm::vec4 m_nameplateTextColor{1.0f, 0.90f, 0.75f, 1.0f};

    // Опциональные текстурные скины
    std::shared_ptr<Texture2D> m_skinTexture;
    std::shared_ptr<Texture2D> m_nameplateSkinTexture;

    // Масштаб шрифтов
    float m_textScale = 0.85f;
    float m_nameplateScale = 0.85f;

    // Анимация индикатора завершения реплики
    float m_blinkTimer = 0.0f;
    float m_bounceOffset = 0.0f;
};

#endif //DARIVN_DIALOGUEBOX_HPP
