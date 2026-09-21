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
