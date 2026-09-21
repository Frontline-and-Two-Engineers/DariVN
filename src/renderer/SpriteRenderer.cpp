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
    if (!m_shader || !texture.isValid()) return;

    m_shader->bind();

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

    m_shader->setMat4("u_Model", model);
    m_shader->setVec4("u_Color", params.color);
    m_shader->setInt("u_Silhouette", params.isSilhouette ? 1 : 0);
    m_shader->setVec4("u_SilhouetteColor", params.silhouetteColor);
    m_shader->setFloat("u_Flash", params.flash);
    m_shader->setFloat("u_Sepia", params.sepia);

    // Включаем альфа-блендинг для прозрачности спрайтов
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    texture.bind(0);
    m_shader->setInt("u_Texture", 0);

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
