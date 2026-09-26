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

#ifndef DARIVN_SPRITERENDERER_HPP
#define DARIVN_SPRITERENDERER_HPP

#include <memory>
#include <glm/glm.hpp>

class Shader;
class Texture2D;

struct SpriteEffectParams {
    glm::vec4 color = glm::vec4(1.0f);
    bool isSilhouette = false;
    glm::vec4 silhouetteColor = glm::vec4(0.08f, 0.09f, 0.14f, 1.0f);
    float flash = 0.0f;
    float sepia = 0.0f;
};

class SpriteRenderer {
public:
    explicit SpriteRenderer(std::shared_ptr<Shader> shader = nullptr);
    ~SpriteRenderer();

    // Запрещаем копирование (RAII для OpenGL буферов)
    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;

    void setShader(std::shared_ptr<Shader> shader) { m_shader = std::move(shader); }
    [[nodiscard]] std::shared_ptr<Shader> getShader() const { return m_shader; }

    // Отрисовка текстурированного спрайта с расширенными эффектами
    void drawSprite(const Texture2D& texture,
                    glm::vec2 position,
                    glm::vec2 size,
                    float rotate,
                    const SpriteEffectParams& params);

    // Базовая отрисовка спрайта
    void drawSprite(const Texture2D& texture, 
                    glm::vec2 position, 
                    glm::vec2 size, 
                    float rotate = 0.0f, 
                    glm::vec4 color = glm::vec4(1.0f));

    // Отрисовка сплошного или полупрозрачного цветного прямоугольника (плашки диалогов, фоны, затемнения)
    void drawRect(glm::vec2 position, 
                  glm::vec2 size, 
                  glm::vec4 color, 
                  float rotate = 0.0f);

private:
    void initRenderData();

private:
    std::shared_ptr<Shader> m_shader;
    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;
};

#endif //DARIVN_SPRITERENDERER_HPP
