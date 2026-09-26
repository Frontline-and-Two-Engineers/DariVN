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

#ifndef DARIVN_TEXTRENDERER_HPP
#define DARIVN_TEXTRENDERER_HPP

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class Shader;

// Структура для хранения метрик отдельного символа
struct CharacterGlyph {
    float x0, y0, x1, y1; // Экранные координаты смещения
    float s0, t0, s1, t1; // Текстурные UV-координаты
    float xAdvance;       // Шаг по горизонтали до следующего символа
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Запрещаем копирование
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    // Загрузка TTF шрифта (с запеканием латиницы и кириллицы в текстурный атлас)
    bool loadFont(const std::string& fontPath, float fontSize = 32.0f);

    // Установка матрицы ортографической проекции
    void setProjection(const glm::mat4& projection);

    // Отрисовка текста UTF-8 (поддерживает русский, английский языки и перенос строк '\n')
    void renderText(std::string_view text, float x, float y, 
                    float scale = 1.0f, 
                    glm::vec4 color = glm::vec4(1.0f));

    // Измерение размеров текста в пикселях (ширина, высота)
    [[nodiscard]] glm::vec2 measureText(std::string_view text, float scale = 1.0f) const;

    // Автоматический перенос строк по ширине (Word Wrapping для UTF-8)
    [[nodiscard]] std::string wrapText(std::string_view text, float maxWidth, float scale = 1.0f) const;

    // Отрисовка текста с автоматическим переносом строк
    void renderWrappedText(std::string_view text, float x, float y, float maxWidth,
                           float scale = 1.0f, glm::vec4 color = glm::vec4(1.0f));

    // Очистка текста от тегов разметки {...} (для подсчета "чистой" длины)
    [[nodiscard]] static std::string stripTags(std::string_view text);

    [[nodiscard]] float getFontSize() const { return m_fontSize; }
    [[nodiscard]] float getLineHeight() const { return m_lineHeight; }
    [[nodiscard]] bool isValid() const { return m_fontTexture != 0; }

private:
    void initRenderData();
    const CharacterGlyph* getGlyph(uint32_t codepoint) const;

private:
    std::shared_ptr<Shader> m_shader;
    unsigned int m_fontTexture = 0;
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    float m_fontSize = 32.0f;
    float m_lineHeight = 36.0f;

    // Глифы: диапазон Latin (32..126) и Cyrillic (0x0400..0x04FF)
    std::vector<CharacterGlyph> m_latinGlyphs;
    std::vector<CharacterGlyph> m_cyrillicGlyphs;
};

#endif //DARIVN_TEXTRENDERER_HPP
