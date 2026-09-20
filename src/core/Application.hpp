#pragma once

#ifndef DARIVN_APPLICATION_HPP
#define DARIVN_APPLICATION_HPP
#include <memory>

class Window;
class RenderPipeline;
class Scene;
class StoryEngine;
class PythonEngine;

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();

private:
    bool init();
    void processInput(float dt);
    void update(float dt);
    void render();
    void shutdown();

private:
    bool m_isRunning = false;
    float m_lastFrameTime = 0.0f;

    std::unique_ptr<Window> m_window;
    std::unique_ptr<RenderPipeline> m_renderPipeline;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<StoryEngine> m_storyEngine;
    std::unique_ptr<PythonEngine> m_pythonEngine;
};


#endif //DARIVN_APPLICATION_HPP
