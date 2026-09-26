cmake_minimum_required(VERSION 3.24)
project(DariVN LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# 1. Автоматическая загрузка и сборка GLFW под текущую платформу (macOS arm64 / Linux / Win)
include(FetchContent)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 2. Исполняемый файл (пока подключаем только ядро)
add_executable(DariVN
        src/main.cpp
        src/core/Application.cpp
        src/core/Application.hpp
        src/core/Config.cpp
        src/core/Config.hpp
        src/core/SaveManager.cpp
        src/core/SaveManager.hpp
        src/core/Window.cpp
        src/core/Window.hpp
        src/core/Input.cpp
        src/core/Input.hpp
        src/renderer/Shader.cpp
        src/renderer/Shader.hpp
        src/renderer/Texture2D.cpp
        src/renderer/Texture2D.hpp
        src/renderer/Framebuffer.cpp
        src/renderer/Framebuffer.hpp
        src/renderer/RenderPipeline.cpp
        src/renderer/RenderPipeline.hpp
        src/renderer/SpriteRenderer.cpp
        src/renderer/SpriteRenderer.hpp
        src/renderer/TextRenderer.cpp
        src/renderer/TextRenderer.hpp
        src/resource/ResourceManager.cpp
        src/resource/ResourceManager.hpp
        src/resource/Localization.cpp
        src/resource/Localization.hpp
        src/scene/Button.cpp
        src/scene/Button.hpp
        src/scene/CharacterSprite.cpp
        src/scene/CharacterSprite.hpp
        src/scene/DialogueBox.cpp
        src/scene/DialogueBox.hpp
        src/script/ScriptCommand.hpp
        src/script/ExpressionEvaluator.cpp
        src/script/ExpressionEvaluator.hpp
        src/script/ScriptParser.cpp
        src/script/ScriptParser.hpp
        src/script/StoryEngine.cpp
        src/script/StoryEngine.hpp
        src/audio/AudioEngine.cpp
        src/audio/AudioEngine.hpp
        src/states/SharedContext.hpp
        src/states/IGameState.hpp
        src/states/StateManager.hpp
        src/states/StateManager.cpp
        src/states/MainMenuState.hpp
        src/states/MainMenuState.cpp
        src/states/GameplayState.hpp
        src/states/GameplayState.cpp
        src/states/PauseMenuState.hpp
        src/states/PauseMenuState.cpp
        src/states/BacklogState.hpp
        src/states/BacklogState.cpp
        src/states/SaveLoadState.hpp
        src/states/SaveLoadState.cpp
        src/states/SettingsState.hpp
        src/states/SettingsState.cpp
        external/GLAD/src/glad.c
)

target_include_directories(DariVN PRIVATE
        src/
        external/GLAD/include
        external/GLM/include
        external/include
)

# 3. Линковка
target_link_libraries(DariVN PRIVATE glfw)

if (APPLE)
    target_link_libraries(DariVN PRIVATE 
        "-framework OpenGL" 
        "-framework CoreAudio" 
        "-framework AudioToolbox" 
        "-framework CoreFoundation"
    )
elseif (WIN32)
    target_link_libraries(DariVN PRIVATE opengl32)
elseif (UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(DariVN PRIVATE OpenGL::GL ${CMAKE_DL_LIBS})
endif()