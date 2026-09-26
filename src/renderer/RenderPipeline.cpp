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

#include "RenderPipeline.hpp"
#include "Shader.hpp"
#include <glad/glad.h>
#include <iostream>

RenderPipeline::RenderPipeline(int virtualWidth, int virtualHeight)
    : m_virtualWidth(virtualWidth), m_virtualHeight(virtualHeight) {
    m_sceneFBO = std::make_unique<Framebuffer>(virtualWidth, virtualHeight);
    m_prevFBO = std::make_unique<Framebuffer>(virtualWidth, virtualHeight);
    initQuad();
}

RenderPipeline::~RenderPipeline() {
    if (m_quadVAO != 0) {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_quadVBO != 0) {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
}

void RenderPipeline::startTransition(TransitionType type, float duration) {
    m_currentTransition = type;
    m_transitionDuration = (duration > 0.01f) ? duration : 1.0f;
    m_transitionTimer = 0.0f;
    m_isTransitioning = true;
}

void RenderPipeline::updateTransition(float deltaTime) {
    if (m_isTransitioning) {
        m_transitionTimer += deltaTime;
        if (m_transitionTimer >= m_transitionDuration) {
            m_transitionTimer = m_transitionDuration;
            m_isTransitioning = false;
        }
    }
}

void RenderPipeline::capturePreviousScene() {
    // Мгновенный перенос текущей сцены в буфер предыдущей (O(1) pointer swap)
    std::swap(m_sceneFBO, m_prevFBO);
}

float RenderPipeline::getTransitionProgress() const {
    return (m_transitionDuration > 0.0f) ? std::min(1.0f, m_transitionTimer / m_transitionDuration) : 1.0f;
}

void RenderPipeline::initQuad() {
    // 2D полноэкранный квад (NDC: от -1.0 до 1.0, UV: от 0.0 до 1.0)
    float quadVertices[] = {
        // Позиция      // Текстурные UV
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,

        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void RenderPipeline::beginScene(float r, float g, float b, float a) {
    if (m_sceneFBO) {
        m_sceneFBO->bind();
        m_sceneFBO->clear(r, g, b, a);
    }
}

void RenderPipeline::endScene() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPipeline::renderToScreen(const PostProcessSettings& settings, int screenWidth, int screenHeight) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 1. Если активен переход между сценами — рисуем через transitionShader
    if (m_isTransitioning && m_transitionShader && m_transitionShader->isValid() && m_prevFBO && m_sceneFBO) {
        m_transitionShader->bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_prevFBO->getColorTexture());
        m_transitionShader->setInt("u_PrevTexture", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_sceneFBO->getColorTexture());
        m_transitionShader->setInt("u_NextTexture", 1);

        m_transitionShader->setFloat("u_Progress", getTransitionProgress());
        m_transitionShader->setInt("u_TransitionType", static_cast<int>(m_currentTransition));

        glBindVertexArray(m_quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    // 2. Стандартный вывод через шейдер пост-процессинга
    if (!m_postShader || !m_postShader->isValid() || !m_sceneFBO) {
        return;
    }

    m_postShader->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneFBO->getColorTexture());
    m_postShader->setInt("u_ScreenTexture", 0);
    m_postShader->setVec2("u_ScreenResolution", glm::vec2(static_cast<float>(m_virtualWidth), static_cast<float>(m_virtualHeight)));

    m_postShader->setFloat("u_BlurStrength", settings.blurStrength);
    m_postShader->setFloat("u_VignetteIntensity", settings.vignetteIntensity);
    m_postShader->setFloat("u_SepiaTone", settings.sepiaTone);
    m_postShader->setVec4("u_ColorTint", settings.colorTint);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderPipeline::resize(int virtualWidth, int virtualHeight) {
    if (virtualWidth <= 0 || virtualHeight <= 0) return;
    m_virtualWidth = virtualWidth;
    m_virtualHeight = virtualHeight;
    if (m_sceneFBO) {
        m_sceneFBO->resize(virtualWidth, virtualHeight);
    }
    if (m_prevFBO) {
        m_prevFBO->resize(virtualWidth, virtualHeight);
    }
}
