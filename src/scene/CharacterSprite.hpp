#pragma once

#ifndef DARIVN_CHARACTERSPRITE_HPP
#define DARIVN_CHARACTERSPRITE_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

class Texture2D;
class SpriteRenderer;
class TextRenderer;

class CharacterSprite {
public:
    CharacterSprite(std::string name, glm::vec2 size = glm::vec2(360.0f, 640.0f));
    ~CharacterSprite() = default;

    // Регистрация текстуры для эмоции
    void addExpression(const std::string& expressionName, std::shared_ptr<Texture2D> texture);
    void addExpression(const std::string& expressionName, const std::string& texturePath);

    // Смена активной эмоции
    void setExpression(const std::string& expressionName);

    // Свободное позиционирование (конкретные X, Y) или слот ("left", "center", "right")
    void setPosition(glm::vec2 targetPos, bool animate = true);
    void setPositionX(float targetX, bool animate = true);
    void setSlotPosition(const std::string& slot, bool animate = true);

    // Масштабирование и автоматическое сохранение пропорций текстуры
    void setScale(float scale);
    [[nodiscard]] float getScale() const { return m_scale; }

    void setTargetHeight(float height);
    [[nodiscard]] float getTargetHeight() const { return m_targetHeight; }

    void setPreserveAspectRatio(bool preserve) { m_preserveAspectRatio = preserve; }
    [[nodiscard]] bool isPreservingAspectRatio() const { return m_preserveAspectRatio; }

    void recalculateSize();

    // Слой отрисовки (Z-index: меньшие слои рисуются сзади, большие — впереди)
    void setLayer(int layer) { m_layer = layer; }
    [[nodiscard]] int getLayer() const { return m_layer; }

    // Отображаемое имя персонажа (например "Алиса" при id "dari")
    void setDisplayName(std::string displayName) { m_displayName = std::move(displayName); }
    [[nodiscard]] const std::string& getDisplayName() const { return m_displayName.empty() ? m_name : m_displayName; }

    // Индивидуальные визуальные эффекты
    void setTint(const glm::vec4& tint) { m_tint = tint; }
    [[nodiscard]] glm::vec4 getTint() const { return m_tint; }

    void setSilhouette(bool enable, const glm::vec4& color = glm::vec4(0.08f, 0.09f, 0.14f, 1.0f)) {
        m_isSilhouette = enable;
        m_silhouetteColor = color;
    }
    [[nodiscard]] bool isSilhouette() const { return m_isSilhouette; }

    void setSepia(float sepia) { m_sepia = sepia; }
    [[nodiscard]] float getSepia() const { return m_sepia; }

    // Анимационные триггеры
    void shake(float duration = 0.4f, float intensity = 15.0f);
    void hop(float duration = 0.3f, float height = 25.0f);
    void flash(float duration = 0.25f);
    void resetEffects();

    // Плавное появление (Fade-in) и исчезновение (Fade-out)
    void fadeIn(float duration = 0.35f);
    void fadeOut(float duration = 0.35f);

    // Обновление интерполяции позиции и прозрачности
    void update(float deltaTime);

    // Отрисовка спрайта
    void render(SpriteRenderer& spriteRenderer, TextRenderer& textRenderer);

    [[nodiscard]] const std::string& getName() const { return m_name; }
    [[nodiscard]] const std::string& getCurrentExpression() const { return m_currentExpression; }
    [[nodiscard]] bool isVisible() const { return m_alpha > 0.005f; }
    [[nodiscard]] float getAlpha() const { return m_alpha; }
    void setAlpha(float alpha) { m_alpha = alpha; m_targetAlpha = alpha; }
    [[nodiscard]] glm::vec2 getPosition() const { return m_position; }
    [[nodiscard]] glm::vec2 getSize() const { return m_size; }

    void setSize(glm::vec2 size) { m_size = size; m_preserveAspectRatio = false; }

private:
    std::string m_name;
    std::string m_displayName;
    std::string m_currentExpression = "normal";
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_expressions;

    glm::vec2 m_position = glm::vec2(460.0f, 120.0f);
    glm::vec2 m_targetPosition = glm::vec2(460.0f, 120.0f);
    glm::vec2 m_size = glm::vec2(360.0f, 580.0f);

    float m_targetHeight = 580.0f;     // Базовая высота персонажа на 720p экране
    float m_scale = 1.0f;              // Пользовательский масштаб (множитель)
    bool m_preserveAspectRatio = true; // Автоматически сохранять соотношение сторон текстуры

    int m_layer = 0; // Слой отрисовки

    float m_alpha = 0.0f;
    float m_targetAlpha = 1.0f;
    float m_fadeSpeed = 3.0f;
    float m_moveSpeed = 10.0f;

    // Визуальные эффекты персонажа
    glm::vec4 m_tint = glm::vec4(1.0f);
    bool m_isSilhouette = false;
    glm::vec4 m_silhouetteColor = glm::vec4(0.08f, 0.09f, 0.14f, 1.0f);
    float m_sepia = 0.0f;

    // Анимационные таймеры
    float m_flashTimer = 0.0f;
    float m_flashDuration = 0.0f;

    float m_shakeTimer = 0.0f;
    float m_shakeDuration = 0.0f;
    float m_shakeIntensity = 0.0f;

    float m_bounceTimer = 0.0f;
    float m_bounceDuration = 0.0f;
    float m_bounceHeight = 0.0f;
};

#endif //DARIVN_CHARACTERSPRITE_HPP
