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

#ifndef DARIVN_WINDOW_HPP
#define DARIVN_WINDOW_HPP

#include <string>

struct GLFWwindow;

class Window {
public:
    Window(int width = 800, int height = 600, const std::string& title = "DariVN");
    ~Window();

    bool is_valid() const;
    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();
    void setVSync(bool enabled);
    bool isKeyJustPressed(int key) const;

    void setFullscreen(bool fullscreen);
    [[nodiscard]] bool isFullscreen() const { return m_isFullscreen; }

    [[nodiscard]] int getWidth() const { return m_width; }
    [[nodiscard]] int getHeight() const { return m_height; }
    void getFramebufferSize(int* width, int* height) const;
    [[nodiscard]] GLFWwindow* getNativeWindow() const { return m_window; }

private:
    GLFWwindow* m_window = nullptr;
    int m_width = 1280;
    int m_height = 720;
    bool m_isFullscreen = false;
    int m_windowedX = 100;
    int m_windowedY = 100;
    int m_windowedWidth = 1280;
    int m_windowedHeight = 720;
};


#endif //DARIVN_WINDOW_HPP
