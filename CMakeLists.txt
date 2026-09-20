cmake_minimum_required(VERSION 4.3)
project(DariVN)

set(CMAKE_CXX_STANDARD 20)

add_executable(DariVN
        src/main.cpp
        src/core/Application.cpp
        src/core/Application.hpp
        src/core/Window.cpp
        src/core/Window.hpp
        src/core/Input.cpp
        src/core/Input.hpp
        src/core/Events.cpp
        src/core/Events.hpp
        src/renderer/RenderPipeline.cpp
        src/renderer/RenderPipeline.hpp
        src/renderer/Framebuffer.cpp
        src/renderer/Framebuffer.hpp
        src/renderer/Shader.cpp
        src/renderer/Shader.hpp
        src/renderer/ShaderManager.cpp
        src/renderer/ShaderManager.hpp
        src/renderer/SpriteRenderer.cpp
        src/renderer/SpriteRenderer.hpp
        src/renderer/TextRenderer.cpp
        src/renderer/TextRenderer.hpp
        src/scene/Scene.cpp
        src/scene/Scene.hpp
        src/scene/CharacterSprite.cpp
        src/scene/CharacterSprite.hpp
        src/scene/DialogueBox.cpp
        src/scene/DialogueBox.hpp
        src/scene/ChoiceMenu.cpp
        src/scene/ChoiceMenu.hpp
        src/script/ScriptCommand.hpp
        src/script/ScriptParser.cpp
        src/script/ScriptParser.hpp
        src/script/StoryEngine.cpp
        src/script/StoryEngine.hpp
        src/scripting_api/PythonEngine.cpp
        src/scripting_api/PythonEngine.hpp
        src/scripting_api/BindingsScene.cpp
        src/scripting_api/BindingsRender.cpp
        src/resource/ResourceManager.cpp
        src/resource/ResourceManager.hpp
)

target_include_directories(DariVN PRIVATE
    src/
    external/GLFW/include
    external/GLAD/include
    external/GLM/include
    external/include
)

target_link_libraries(DariVN PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/GLFW/lib/libglfw3.a)
