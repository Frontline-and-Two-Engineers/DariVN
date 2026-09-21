#pragma once

#ifndef DARIVN_BUTTON_HPP
#define DARIVN_BUTTON_HPP

#include <string>
#include <functional>
#include <glm/glm.hpp>

class SpriteRenderer;
class TextRenderer;

class Button {
public:
    Button(glm::vec2 position, glm::vec2 size, std::string text, std::function<void()> onClick);

    bool update(glm::vec2 mousePos, bool mouseJustPressed);
    void render(SpriteRenderer& spriteRenderer, TextRenderer& textRenderer);

    void setPosition(glm::vec2 pos) { m_position = pos; }
    void setSize(glm::vec2 size) { m_size = size; }
    void setText(std::string text) { m_text = std::move(text); }

    [[nodiscard]] bool isHovered(glm::vec2 mousePos) const {
        return mousePos.x >= m_position.x && mousePos.x <= m_position.x + m_size.x &&
               mousePos.y >= m_position.y && mousePos.y <= m_position.y + m_size.y;
    }

    static void setGlobalClickCallback(std::function<void()> cb) { s_clickCallback = std::move(cb); }

private:
    glm::vec2 m_position;
    glm::vec2 m_size;
    std::string m_text;
    std::function<void()> m_onClick;
    bool m_hovered = false;

    static inline std::function<void()> s_clickCallback = nullptr;
};

#endif //DARIVN_BUTTON_HPP
