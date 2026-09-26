#pragma once

#ifndef DARIVN_INPUT_HPP
#define DARIVN_INPUT_HPP

#include <glm/glm.hpp>
#include <array>

struct GLFWwindow;

class Input {
public:
    Input() = delete;

    static void init(GLFWwindow* window, float virtualWidth = 1280.0f, float virtualHeight = 720.0f);
    static void update();

    // Клавиатура
    static bool isKeyDown(int key);
    static bool isKeyJustPressed(int key);
    static bool isKeyJustReleased(int key);

    // Мышь
    static bool isMouseButtonDown(int button);
    static bool isMouseButtonJustPressed(int button);
    static bool isMouseButtonJustReleased(int button);

    static glm::vec2 getMousePosition();
    static float getMouseX();
    static float getMouseY();

    // Колесико мыши
    static float getMouseScrollY();
    static float getMouseScrollX();

    static void setVirtualResolution(float width, float height);

private:
    static inline GLFWwindow* s_window = nullptr;
    static inline float s_virtualWidth = 1280.0f;
    static inline float s_virtualHeight = 720.0f;

    static inline std::array<bool, 512> s_currentKeys{};
    static inline std::array<bool, 512> s_previousKeys{};

    static inline std::array<bool, 16> s_currentMouseButtons{};
    static inline std::array<bool, 16> s_previousMouseButtons{};

    static inline glm::vec2 s_mousePos{0.0f, 0.0f};
    static inline float s_scrollX = 0.0f;
    static inline float s_scrollY = 0.0f;
};

#endif //DARIVN_INPUT_HPP
