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

#include "CharacterSprite.hpp"
#include "renderer/Texture2D.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"
#include "resource/ResourceManager.hpp"

#include <algorithm>

CharacterSprite::CharacterSprite(std::string name, glm::vec2 size)
    : m_name(std::move(name)), m_size(size), m_targetHeight(size.y) {}

void CharacterSprite::setScale(float scale) {
    m_scale = scale;
    recalculateSize();
}

void CharacterSprite::setTargetHeight(float height) {
    m_targetHeight = height;
    recalculateSize();
}

void CharacterSprite::recalculateSize() {
    if (!m_preserveAspectRatio) return;

    std::shared_ptr<Texture2D> tex = nullptr;
    auto it = m_expressions.find(m_currentExpression);
    if (it != m_expressions.end() && it->second && it->second->isValid()) {
        tex = it->second;
    } else {
        auto normalIt = m_expressions.find("normal");
        if (normalIt != m_expressions.end() && normalIt->second && normalIt->second->isValid()) {
            tex = normalIt->second;
        }
    }

    if (tex && tex->getWidth() > 0 && tex->getHeight() > 0) {
        float aspect = static_cast<float>(tex->getWidth()) / static_cast<float>(tex->getHeight());
        m_size.y = m_targetHeight * m_scale;
        m_size.x = m_size.y * aspect;
    }
}

void CharacterSprite::addExpression(const std::string& expressionName, std::shared_ptr<Texture2D> texture) {
    if (texture) {
        m_expressions[expressionName] = std::move(texture);
        recalculateSize();
    }
}

void CharacterSprite::addExpression(const std::string& expressionName, const std::string& texturePath) {
    auto tex = ResourceManager::loadTexture(m_name + "_" + expressionName, texturePath);
    if (tex) {
        m_expressions[expressionName] = tex;
        recalculateSize();
    }
}

void CharacterSprite::setExpression(const std::string& expressionName) {
    m_currentExpression = expressionName;
    recalculateSize();
}

void CharacterSprite::setSlotPosition(const std::string& slot, bool animate) {
    recalculateSize();
    float centerX = 640.0f;
    if (slot == "left") {
        centerX = 320.0f;
    } else if (slot == "right") {
        centerX = 960.0f;
    }

    glm::vec2 target(centerX - m_size.x * 0.5f, 120.0f);
    setPosition(target, animate);
}

void CharacterSprite::setPosition(glm::vec2 targetPos, bool animate) {
    m_targetPosition = targetPos;
    if (!animate) {
        m_position = targetPos;
    }
}

void CharacterSprite::setPositionX(float targetX, bool animate) {
    m_targetPosition.x = targetX;
    if (!animate) {
        m_position.x = targetX;
    }
}

void CharacterSprite::shake(float duration, float intensity) {
    m_shakeTimer = duration;
    m_shakeDuration = duration;
    m_shakeIntensity = intensity;
}

void CharacterSprite::hop(float duration, float height) {
    m_bounceTimer = duration;
    m_bounceDuration = duration;
    m_bounceHeight = height;
}

void CharacterSprite::flash(float duration) {
    m_flashTimer = duration;
    m_flashDuration = duration;
}

void CharacterSprite::resetEffects() {
    m_tint = glm::vec4(1.0f);
    m_isSilhouette = false;
    m_silhouetteColor = glm::vec4(0.08f, 0.09f, 0.14f, 1.0f);
    m_sepia = 0.0f;
    m_flashTimer = 0.0f;
    m_shakeTimer = 0.0f;
    m_bounceTimer = 0.0f;
}

void CharacterSprite::fadeIn(float duration) {
    m_targetAlpha = 1.0f;
    m_fadeSpeed = (duration > 0.0f) ? (1.0f / duration) : 100.0f;
}

void CharacterSprite::fadeOut(float duration) {
    m_targetAlpha = 0.0f;
    m_fadeSpeed = (duration > 0.0f) ? (1.0f / duration) : 100.0f;
}

void CharacterSprite::update(float deltaTime) {
    // 1. Плавная интерполяция позиции (плавное скольжение)
    float moveStep = std::min(1.0f, m_moveSpeed * deltaTime);
    m_position += (m_targetPosition - m_position) * moveStep;

    // 2. Плавная интерполяция прозрачности (Fade-in / Fade-out)
    if (m_alpha < m_targetAlpha) {
        m_alpha = std::min(m_targetAlpha, m_alpha + m_fadeSpeed * deltaTime);
    } else if (m_alpha > m_targetAlpha) {
        m_alpha = std::max(m_targetAlpha, m_alpha - m_fadeSpeed * deltaTime);
    }

    // 3. Обновление таймеров динамических анимаций
    if (m_shakeTimer > 0.0f) {
        m_shakeTimer = std::max(0.0f, m_shakeTimer - deltaTime);
    }
    if (m_bounceTimer > 0.0f) {
        m_bounceTimer = std::max(0.0f, m_bounceTimer - deltaTime);
    }
    if (m_flashTimer > 0.0f) {
        m_flashTimer = std::max(0.0f, m_flashTimer - deltaTime);
    }
}

void CharacterSprite::render(SpriteRenderer& spriteRenderer, TextRenderer& textRenderer) {
    if (!isVisible()) return;

    // Расчёт динамических смещений для анимаций
    glm::vec2 renderPos = m_position;

    // Смещение дрожания (Shake)
    if (m_shakeTimer > 0.0f && m_shakeDuration > 0.0f) {
        float progress = m_shakeTimer / m_shakeDuration;
        float offset = std::sin(m_shakeTimer * 50.0f) * m_shakeIntensity * progress;
        renderPos.x += offset;
    }

    // Смещение подскока (Hop)
    if (m_bounceTimer > 0.0f && m_bounceDuration > 0.0f) {
        float t = (m_bounceDuration - m_bounceTimer) / m_bounceDuration; // 0 -> 1
        float hopY = -std::sin(t * 3.14159265f) * m_bounceHeight;
        renderPos.y += hopY;
    }

    float flashVal = (m_flashDuration > 0.0f) ? (m_flashTimer / m_flashDuration) : 0.0f;

    SpriteEffectParams params;
    params.color = m_tint * glm::vec4(1.0f, 1.0f, 1.0f, m_alpha);
    params.isSilhouette = m_isSilhouette;
    params.silhouetteColor = m_silhouetteColor;
    params.flash = flashVal;
    params.sepia = m_sepia;

    // Ищем текстуру для текущей эмоции
    std::shared_ptr<Texture2D> texture = nullptr;
    auto it = m_expressions.find(m_currentExpression);
    if (it != m_expressions.end() && it->second && it->second->isValid()) {
        texture = it->second;
    } else {
        // Пробуем взять дефолтную "normal"
        auto normalIt = m_expressions.find("normal");
        if (normalIt != m_expressions.end() && normalIt->second && normalIt->second->isValid()) {
            texture = normalIt->second;
        }
    }

    // Если есть реальная текстура спрайта — рисуем её с правильными пропорциями и эффектами
    if (texture) {
        if (m_preserveAspectRatio && texture->getWidth() > 0 && texture->getHeight() > 0) {
            float aspect = static_cast<float>(texture->getWidth()) / static_cast<float>(texture->getHeight());
            m_size.y = m_targetHeight * m_scale;
            m_size.x = m_size.y * aspect;
        }
        spriteRenderer.drawSprite(*texture, renderPos, m_size, 0.0f, params);
        return;
    }

    // Иначе рисуем стилизованный силуэт-плейсхолдер
    glm::vec4 bodyColor = m_isSilhouette ? m_silhouetteColor * m_alpha : (glm::vec4(0.20f, 0.25f, 0.38f, 0.90f * m_alpha) * m_tint);
    glm::vec4 headColor = m_isSilhouette ? m_silhouetteColor * m_alpha : (glm::vec4(0.25f, 0.32f, 0.48f, 0.95f * m_alpha) * m_tint);
    glm::vec4 borderColor = glm::vec4(0.45f, 0.65f, 0.95f, 0.85f * m_alpha) * m_tint;

    // Тело
    spriteRenderer.drawRect(renderPos + glm::vec2(25.0f, 100.0f), glm::vec2(m_size.x - 50.0f, m_size.y - 100.0f), bodyColor);
    // Голова
    float headSize = 120.0f;
    spriteRenderer.drawRect(renderPos + glm::vec2((m_size.x - headSize) * 0.5f, 0.0f), glm::vec2(headSize, headSize), headColor);

    if (!m_isSilhouette) {
        // Рамка вокруг тела
        spriteRenderer.drawRect(renderPos + glm::vec2(25.0f, 100.0f), glm::vec2(m_size.x - 50.0f, 2.0f), borderColor);

        // Имя персонажа и эмоция
        std::string label = getDisplayName();
        if (m_currentExpression != "normal") {
            label += " (" + m_currentExpression + ")";
        }

        glm::vec2 labelSize = textRenderer.measureText(label, 0.75f);
        float labelX = renderPos.x + (m_size.x - labelSize.x) * 0.5f;
        float labelY = renderPos.y + 240.0f;
        textRenderer.renderText(label, labelX, labelY, 0.75f, glm::vec4(1.0f, 1.0f, 1.0f, m_alpha) * m_tint);
    }
}
