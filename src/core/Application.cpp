#include "Application.hpp"

#include "core/Window.hpp"
#include "renderer/RenderPipeline.hpp"
#include "scene/Scene.hpp"
#include "script/StoryEngine.hpp"
#include "scripting_api/PythonEngine.hpp"

#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

Application::Application() = default;

Application::~Application() {
    shutdown();
}

bool Application::init() {
    m_window = std::make_unique<Window>();
    if (!m_window->is_valid()) {
        std::cerr << "WINDOW INITIALIZATION ERROR" << std::endl;
        return false;
    }

    m_renderPipeline = std::make_unique<RenderPipeline>();

    m_scene = std::make_unique<Scene>();
    m_storyEngine = std::make_unique<StoryEngine>();

    m_pythonEngine = std::make_unique<PythonEngine>();
    m_pythonEngine->init();

    // TODO: разобраться с инициализацией до конца

    m_isRunning = true;
    m_lastFrameTime = static_cast<float>(glfwGetTime());
    return true;
}

void Application::run() {
    if (!init()) {
        throw std::runtime_error("Application initialization failed");
    }

    while (m_isRunning && !m_window->shouldClose()) {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = deltaTime;

        processInput(deltaTime);
        update(deltaTime);
        render();

        m_window->swapBuffers();
    }
}

void Application::processInput(float deltaTime) {
    m_window->pollEvents();

    if (m_window->isKeyJustPressed(GLFW_KEY_SPACE)) {
        m_storyEngine->nextStep();
    }
}
void Application::update(float deltaTime) {
    m_storyEngine->update(deltaTime);

    m_pythonEngine->update(deltaTime);

    m_scene->update(deltaTime);
}