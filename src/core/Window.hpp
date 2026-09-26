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
