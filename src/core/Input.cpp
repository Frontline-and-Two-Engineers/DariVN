#include "Input.hpp"
#include <GLFW/glfw3.h>

void Input::init(GLFWwindow* window, float virtualWidth, float virtualHeight) {
    s_window = window;
    s_virtualWidth = virtualWidth;
    s_virtualHeight = virtualHeight;

    s_currentKeys.fill(false);
    s_previousKeys.fill(false);
    s_currentMouseButtons.fill(false);
    s_previousMouseButtons.fill(false);

    // Устанавливаем начальную позицию мыши
    double xpos = 0.0, ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);
    int winW = 1, winH = 1;
    glfwGetWindowSize(window, &winW, &winH);
    if (winW > 0 && winH > 0) {
        s_mousePos.x = static_cast<float>(xpos) * (s_virtualWidth / static_cast<float>(winW));
        s_mousePos.y = static_cast<float>(ypos) * (s_virtualHeight / static_cast<float>(winH));
    }

    glfwSetKeyCallback(window, [](GLFWwindow*, int key, int, int action, int) {
        if (key >= 0 && key < static_cast<int>(s_currentKeys.size())) {
            if (action == GLFW_PRESS) {
                s_currentKeys[key] = true;
            } else if (action == GLFW_RELEASE) {
                s_currentKeys[key] = false;
            }
        }
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow*, int button, int action, int) {
        if (button >= 0 && button < static_cast<int>(s_currentMouseButtons.size())) {
            if (action == GLFW_PRESS) {
                s_currentMouseButtons[button] = true;
            } else if (action == GLFW_RELEASE) {
                s_currentMouseButtons[button] = false;
            }
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double xpos, double ypos) {
        int winW = 1, winH = 1;
        glfwGetWindowSize(win, &winW, &winH);
        if (winW > 0 && winH > 0) {
            s_mousePos.x = static_cast<float>(xpos) * (s_virtualWidth / static_cast<float>(winW));
            s_mousePos.y = static_cast<float>(ypos) * (s_virtualHeight / static_cast<float>(winH));
        }
    });

    glfwSetScrollCallback(window, [](GLFWwindow*, double xoffset, double yoffset) {
        s_scrollX += static_cast<float>(xoffset);
        s_scrollY += static_cast<float>(yoffset);
    });
}

void Input::update() {
    s_previousKeys = s_currentKeys;
    s_previousMouseButtons = s_currentMouseButtons;
    s_scrollX = 0.0f;
    s_scrollY = 0.0f;
}

float Input::getMouseScrollY() {
    return s_scrollY;
}

float Input::getMouseScrollX() {
    return s_scrollX;
}

bool Input::isKeyDown(int key) {
    if (key >= 0 && key < static_cast<int>(s_currentKeys.size())) {
        return s_currentKeys[key];
    }
    return false;
}

bool Input::isKeyJustPressed(int key) {
    if (key >= 0 && key < static_cast<int>(s_currentKeys.size())) {
        return s_currentKeys[key] && !s_previousKeys[key];
    }
    return false;
}

bool Input::isKeyJustReleased(int key) {
    if (key >= 0 && key < static_cast<int>(s_currentKeys.size())) {
        return !s_currentKeys[key] && s_previousKeys[key];
    }
    return false;
}

bool Input::isMouseButtonDown(int button) {
    if (button >= 0 && button < static_cast<int>(s_currentMouseButtons.size())) {
        return s_currentMouseButtons[button];
    }
    return false;
}

bool Input::isMouseButtonJustPressed(int button) {
    if (button >= 0 && button < static_cast<int>(s_currentMouseButtons.size())) {
        return s_currentMouseButtons[button] && !s_previousMouseButtons[button];
    }
    return false;
}

bool Input::isMouseButtonJustReleased(int button) {
    if (button >= 0 && button < static_cast<int>(s_currentMouseButtons.size())) {
        return !s_currentMouseButtons[button] && s_previousMouseButtons[button];
    }
    return false;
}

glm::vec2 Input::getMousePosition() {
    return s_mousePos;
}

float Input::getMouseX() {
    return s_mousePos.x;
}

float Input::getMouseY() {
    return s_mousePos.y;
}

void Input::setVirtualResolution(float width, float height) {
    s_virtualWidth = width;
    s_virtualHeight = height;
}
