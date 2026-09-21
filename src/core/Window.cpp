#include "Window.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined (__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return;
    }

    glfwSwapInterval(1);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

bool Window::is_valid() const { return m_window != nullptr; }
bool Window::shouldClose() const { return glfwWindowShouldClose(m_window); }
void Window::swapBuffers() { glfwSwapBuffers(m_window); }
void Window::pollEvents() { glfwPollEvents(); }
void Window::setVSync(bool enabled) { glfwSwapInterval(enabled ? 1 : 0); }
void Window::getFramebufferSize(int* width, int* height) const {
    if (m_window) {
        glfwGetFramebufferSize(m_window, width, height);
    } else {
        if (width) *width = m_width;
        if (height) *height = m_height;
    }
}
bool Window::isKeyJustPressed(int key) const { return glfwGetKey(m_window, key) == GLFW_PRESS; }