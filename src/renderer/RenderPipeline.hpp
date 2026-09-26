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

#ifndef DARIVN_RENDERPIPELINE_HPP
#define DARIVN_RENDERPIPELINE_HPP

#include <memory>
#include <glm/glm.hpp>
#include "Framebuffer.hpp"

class Shader;

struct PostProcessSettings {
    float blurStrength = 0.0f;       // 0.0 = нет размытия, 1.0+ = мягкое размытие
    float vignetteIntensity = 0.0f;  // 0.0 = нет виньетки, 0.5-0.9 = затемнение краев
    float sepiaTone = 0.0f;          // 0.0 = обычный цвет, 1.0 = теплая сепия (воспоминания)
    glm::vec4 colorTint = glm::vec4(1.0f); // Цветовой фильтр (r, g, b, a)

    void reset() {
        blurStrength = 0.0f;
        vignetteIntensity = 0.0f;
        sepiaTone = 0.0f;
        colorTint = glm::vec4(1.0f);
    }
};

enum class TransitionType {
    Crossfade = 0,
    Fade = 1,
    Dissolve = 2
};

class RenderPipeline {
public:
    RenderPipeline(int virtualWidth = 1280, int virtualHeight = 720);
    ~RenderPipeline();

    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    void setPostProcessShader(std::shared_ptr<Shader> shader) { m_postShader = std::move(shader); }
    void setTransitionShader(std::shared_ptr<Shader> shader) { m_transitionShader = std::move(shader); }

    // Управление переходами между сценами
    void startTransition(TransitionType type, float duration = 1.0f);
    void updateTransition(float deltaTime);
    void capturePreviousScene();
    [[nodiscard]] bool isTransitionActive() const { return m_isTransitioning; }
    [[nodiscard]] float getTransitionProgress() const;

    // Начало рендеринга сцены во внеэкранный буфер кадра
    void beginScene(float r = 0.08f, float g = 0.10f, float b = 0.15f, float a = 1.0f);

    // Завершение рендеринга сцены
    void endScene();

    // Отрисовка полноэкранного квада на монитор с применением пост-процессинга и переходов
    void renderToScreen(const PostProcessSettings& settings, int screenWidth, int screenHeight);

    // Изменение виртуального разрешения
    void resize(int virtualWidth, int virtualHeight);

    [[nodiscard]] Framebuffer& getSceneFBO() { return *m_sceneFBO; }
    [[nodiscard]] Framebuffer& getPrevFBO() { return *m_prevFBO; }

private:
    void initQuad();

private:
    int m_virtualWidth;
    int m_virtualHeight;

    std::unique_ptr<Framebuffer> m_sceneFBO;
    std::unique_ptr<Framebuffer> m_prevFBO;
    std::shared_ptr<Shader> m_postShader;
    std::shared_ptr<Shader> m_transitionShader;

    bool m_isTransitioning = false;
    float m_transitionTimer = 0.0f;
    float m_transitionDuration = 1.0f;
    TransitionType m_currentTransition = TransitionType::Crossfade;

    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;
};

#endif //DARIVN_RENDERPIPELINE_HPP
