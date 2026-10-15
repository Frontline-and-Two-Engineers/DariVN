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

#include "SpriteRenderer.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include "resource/ResourceManager.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

SpriteRenderer::SpriteRenderer(std::shared_ptr<Shader> shader)
    : m_shader(std::move(shader)) {
    initRenderData();
}

SpriteRenderer::~SpriteRenderer() {
    if (m_quadVAO != 0) {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_quadVBO != 0) {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
}

void SpriteRenderer::initRenderData() {
    // Вершины квада: <vec2 position, vec2 texCoords>
    float vertices[] = { 
        // pos
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 
        // tex
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(m_quadVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SpriteRenderer::drawSprite(const Texture2D& texture, 
                                glm::vec2 position, 
                                glm::vec2 size, 
                                float rotate, 
                                const SpriteEffectParams& params) {
    drawSprite(texture, position, size, rotate, params, nullptr, {}, 0.0f);
}

void SpriteRenderer::drawSprite(const Texture2D& texture,
                                glm::vec2 position,
                                glm::vec2 size,
                                float rotate,
                                const SpriteEffectParams& params,
                                std::shared_ptr<Shader> customShader,
                                const std::unordered_map<std::string, ShaderUniformValue>& customUniforms,
                                float totalTime) {
    auto activeShader = customShader ? customShader : m_shader;
    if (!activeShader || !texture.isValid()) return;

    activeShader->bind();

    glm::mat4 model = glm::mat4(1.0f);
    // 1. Позиционирование на экране
    model = glm::translate(model, glm::vec3(position, 0.0f));

    // 2. Вращение относительно центра спрайта
    if (rotate != 0.0f) {
        model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f));
        model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f));
    }

    // 3. Масштабирование по размеру
    model = glm::scale(model, glm::vec3(size, 1.0f));

    activeShader->setMat4("u_Projection", m_projection);
    activeShader->setMat4("u_Model", model);
    activeShader->setVec4("u_Color", params.color);
    activeShader->setInt("u_Silhouette", params.isSilhouette ? 1 : 0);
    activeShader->setVec4("u_SilhouetteColor", params.silhouetteColor);
    activeShader->setFloat("u_Flash", params.flash);
    activeShader->setFloat("u_Sepia", params.sepia);
    activeShader->setFloat("u_Time", totalTime);
    activeShader->setVec2("u_Resolution", size);

    // Применяем пользовательские uniform-переменные
    for (const auto& [uName, uVal] : customUniforms) {
        if (uVal.count == 0) activeShader->setInt(uName, uVal.iVal);
        else if (uVal.count == 1) activeShader->setFloat(uName, uVal.f[0]);
        else if (uVal.count == 2) activeShader->setVec2(uName, glm::vec2(uVal.f[0], uVal.f[1]));
        else if (uVal.count == 3) activeShader->setVec3(uName, glm::vec3(uVal.f[0], uVal.f[1], uVal.f[2]));
        else if (uVal.count == 4) activeShader->setVec4(uName, glm::vec4(uVal.f[0], uVal.f[1], uVal.f[2], uVal.f[3]));
    }

    // Включаем альфа-блендинг для прозрачности спрайтов
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    texture.bind(0);
    activeShader->setInt("u_Texture", 0);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void SpriteRenderer::drawSprite(const Texture2D& texture, 
                                glm::vec2 position, 
                                glm::vec2 size, 
                                float rotate, 
                                glm::vec4 color) {
    SpriteEffectParams params;
    params.color = color;
    drawSprite(texture, position, size, rotate, params);
}

void SpriteRenderer::drawRect(glm::vec2 position, glm::vec2 size, glm::vec4 color, float rotate) {
    auto whiteTex = ResourceManager::getWhiteTexture();
    if (whiteTex) {
        drawSprite(*whiteTex, position, size, rotate, color);
    }
}
