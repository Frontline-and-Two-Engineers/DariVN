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
