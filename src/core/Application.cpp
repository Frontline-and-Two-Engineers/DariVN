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

#include "Application.hpp"

#include "core/Window.hpp"
#include "core/Input.hpp"
#include "core/Config.hpp"
#include "core/SaveManager.hpp"
#include "renderer/Shader.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"
#include "renderer/RenderPipeline.hpp"
#include "resource/ResourceManager.hpp"
#include "resource/Localization.hpp"
#include "scene/CharacterSprite.hpp"
#include "scene/DialogueBox.hpp"
#include "scene/Button.hpp"
#include "script/StoryEngine.hpp"
#include "scripting_api/PythonEngine.hpp"
#include "audio/AudioEngine.hpp"
#include "states/SharedContext.hpp"
#include "states/StateManager.hpp"
#include "states/MainMenuState.hpp"
#include "states/GameplayState.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

// Возвращает абсолютный путь к папке, где лежит исполняемый файл
static std::filesystem::path getExecutableDir() {
    static std::filesystem::path s_exeDir = []() {
        try {
#if defined(__APPLE__)
            char path[1024];
            uint32_t size = sizeof(path);
            if (_NSGetExecutablePath(path, &size) == 0) {
                return std::filesystem::canonical(path).parent_path();
            }
#elif defined(_WIN32)
            wchar_t path[MAX_PATH];
            if (GetModuleFileNameW(NULL, path, MAX_PATH) > 0) {
                return std::filesystem::canonical(path).parent_path();
            }
#else
            char path[1024];
            ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
            if (count != -1) {
                path[count] = '\0';
                return std::filesystem::canonical(path).parent_path();
            }
#endif
        } catch (...) {}
        return std::filesystem::current_path();
    }();
    return s_exeDir;
}

// Умный поиск ассетов с поддержкой автономных билдов и запуска из любой директории:
// 1. Относительно текущей рабочей папки (CWD)
// 2. Рядом с исполняемым файлом (автономные билды / запуск из Finder)
// 3. На уровень выше бинарника (cmake-build-debug/ -> корень проекта)
// 4. На уровень выше CWD
static std::string resolveAsset(const std::string& relativePath) {
    if (relativePath.empty()) return "";

    std::filesystem::path rel(relativePath);
    if (rel.is_absolute() && std::filesystem::exists(rel)) {
        return rel.string();
    }

    // 1. В текущей директории
    if (std::filesystem::exists(rel)) {
        return rel.string();
    }

    // 2. В папке исполняемого файла
    auto exeDir = getExecutableDir();
    auto fromExe = exeDir / rel;
    if (std::filesystem::exists(fromExe)) {
        return fromExe.string();
    }

    // 3. На уровень выше папки бинарника (для запуска из cmake-build-debug/)
    auto fromExeParent = exeDir.parent_path() / rel;
    if (std::filesystem::exists(fromExeParent)) {
        return fromExeParent.string();
    }

    // 4. На уровень выше текущей папки
    auto fromCwdParent = std::filesystem::path("..") / rel;
    if (std::filesystem::exists(fromCwdParent)) {
        return fromCwdParent.string();
    }

    return (exeDir / rel).string();
}

Application::Application() = default;

Application::~Application() {
    shutdown();
}

CharacterSprite& Application::getOrCreateCharacter(const std::string& name) {
    auto it = m_characters.find(name);
    if (it != m_characters.end()) {
        return *it->second;
    }
    auto sprite = std::make_unique<CharacterSprite>(name, glm::vec2(360.0f, 640.0f));
    auto* ptr = sprite.get();
    m_characters[name] = std::move(sprite);
    return *ptr;
}

bool Application::init() {
    std::cout << "[Application] Initializing DariVN Engine..." << std::endl;

    // Автоматическая подстройка рабочей директории:
    // Если игра запущена двойным кликом в Finder или из другой папки консоли,
    // переключаем рабочую папку на директорию бинарника (где лежит assets/).
    try {
        if (!std::filesystem::exists("assets/config.vn") && !std::filesystem::exists("config.vn")) {
            auto exeDir = getExecutableDir();
            if (std::filesystem::exists(exeDir / "assets/config.vn") || std::filesystem::exists(exeDir / "config.vn")) {
                std::filesystem::current_path(exeDir);
                std::cout << "[Application] Adjusted working directory to: " << exeDir << std::endl;
            } else if (std::filesystem::exists(exeDir.parent_path() / "assets/config.vn")) {
                std::filesystem::current_path(exeDir.parent_path());
                std::cout << "[Application] Adjusted working directory to: " << exeDir.parent_path() << std::endl;
            }
        }
    } catch (...) {}

    // 1. Загрузка конфигурационного файла (config.vn или assets/config.vn)
    std::string configPath = resolveAsset("config.vn");
    if (!std::filesystem::exists(configPath)) {
        configPath = resolveAsset("assets/config.vn");
    }
    Config::load(configPath);

    // 2. Загрузка пользовательских настроек (имеют высший приоритет над config.vn)
    Config::setUserSettingsPath("saves/settings.vn");
    Config::loadUserSettings();

    int winWidth = Config::getInt("window.width", 1280);
    int winHeight = Config::getInt("window.height", 720);
    std::string winTitle = Config::getString("window.title", "DariVN - Visual Novel Engine");

    m_window = std::make_unique<Window>(winWidth, winHeight, winTitle);
    if (!m_window->is_valid()) {
        std::cerr << "[Application] Window initialization error" << std::endl;
        return false;
    }
    m_window->setVSync(Config::getBool("window.vsync", true));
    if (Config::getBool("window.fullscreen", false)) {
        m_window->setFullscreen(true);
    }

    // Инициализация подсистемы ввода с масштабированием под виртуальный экран
    Input::init(m_window->getNativeWindow(), static_cast<float>(winWidth), static_cast<float>(winHeight));

    // Динамическая загрузка всех локализаций, указанных в конфиге (locale.<код>)
    std::string defLocale = Config::getString("locale.default", "ru");
    Localization::setDefaultLanguage(defLocale);

    auto localeEntries = Config::getEntriesWithPrefix("locale.");
    for (const auto& [key, path] : localeEntries) {
        if (key == "locale.default") continue;
        std::string langCode = key.substr(7);
        if (!langCode.empty()) {
            Localization::loadLocale(langCode, resolveAsset(path));
        }
    }
    if (Localization::getAvailableLanguages().empty()) {
        Localization::loadLocale("ru", resolveAsset("assets/locales/ru.ini"));
        Localization::loadLocale("en", resolveAsset("assets/locales/en.ini"));
    }
    Localization::setLanguage(defLocale);

    // Загружаем базовый шейдер спрайтов
    auto spriteShader = ResourceManager::loadShader(
        "sprite", 
        resolveAsset("assets/shaders/sprite.vert"), 
        resolveAsset("assets/shaders/sprite.frag")
    );

    if (!spriteShader || !spriteShader->isValid()) {
        std::cerr << "[Application] Failed to load sprite shader" << std::endl;
        return false;
    }

    // 2D ортографическая проекция: (0,0) - верхний левый угол
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(winWidth), static_cast<float>(winHeight), 0.0f, -1.0f, 1.0f);
    spriteShader->bind();
    spriteShader->setMat4("u_Projection", projection);

    // Создаем SpriteRenderer
    m_spriteRenderer = std::make_unique<SpriteRenderer>(spriteShader);

    // Создаем TextRenderer и загружаем шрифт
    std::string fontPath = Config::getString("font.path", "assets/fonts/roboto.ttf");
    float fontSize = Config::getFloat("font.size", 28.0f);
    m_textRenderer = std::make_unique<TextRenderer>();
    m_textRenderer->loadFont(resolveAsset(fontPath), fontSize);
    m_textRenderer->setProjection(projection);

    // Создаем диалоговое окно DialogueBox
    m_dialogueBox = std::make_unique<DialogueBox>();

    // Инициализируем конвейер пост-процессинга и загружаем экранный шейдер
    auto postShader = ResourceManager::loadShader(
        "postprocess",
        resolveAsset("assets/shaders/postprocess.vert"),
        resolveAsset("assets/shaders/postprocess.frag")
    );
    m_renderPipeline = std::make_unique<RenderPipeline>(winWidth, winHeight);
    if (postShader && postShader->isValid()) {
        m_renderPipeline->setPostProcessShader(postShader);
    }

    auto transShader = ResourceManager::loadShader(
        "transition",
        resolveAsset("assets/shaders/postprocess.vert"),
        resolveAsset("assets/shaders/transition.frag")
    );
    if (transShader && transShader->isValid()) {
        m_renderPipeline->setTransitionShader(transShader);
    }

    // Инициализируем аудиосистему
    m_audioEngine = std::make_unique<AudioEngine>();
    if (!m_audioEngine->init()) {
        std::cerr << "[Application] Warning: AudioEngine initialization failed" << std::endl;
    } else {
        applyAudioSettings();

        float defaultBgmCrossfade = Config::getFloat("audio.bgm_crossfade", 1.5f);
        m_audioEngine->setDefaultBGMFade(defaultBgmCrossfade);

        m_clickSoundPath = Config::getString("ui.click_sound", "assets/audio/sfx/click.wav");
        Button::setGlobalClickCallback([this]() {
            if (m_audioEngine && !m_clickSoundPath.empty()) {
                m_audioEngine->playSound(resolveAsset(m_clickSoundPath));
            }
        });

        std::string menuMusic = Config::getString("menu.music", "");
        if (!menuMusic.empty()) {
            m_audioEngine->playMusic(resolveAsset(menuMusic), true, defaultBgmCrossfade);
        }
    }

    // Инициализируем StoryEngine и загружаем стартовый сценарий
    m_storyEngine = std::make_unique<StoryEngine>();
    m_storyEngine->setAssetResolver([](const std::string& path) {
        return resolveAsset(path);
    });
    m_storyEngine->setScriptChangeHandler([this](const std::string& newScript) {
        m_characters.clear();
    });
    m_storyEngine->setTransitionEventHandler([this](const std::string& typeStr, float duration) {
        if (m_renderPipeline) {
            m_renderPipeline->capturePreviousScene();
            TransitionType t = TransitionType::Crossfade;
            if (typeStr == "fade") t = TransitionType::Fade;
            else if (typeStr == "dissolve") t = TransitionType::Dissolve;
            m_renderPipeline->startTransition(t, duration);
        }
    });
    m_storyEngine->setAudioEventHandler([this](const StoryEngine::AudioEvent& event) {
        if (!m_audioEngine) return;
        switch (event.type) {
            case StoryEngine::AudioEvent::Type::PlayMusic:
                m_audioEngine->playMusic(resolveAsset(event.path), event.loop, event.fade, event.volume);
                break;
            case StoryEngine::AudioEvent::Type::StopMusic:
                m_audioEngine->stopMusic(event.fade);
                break;
            case StoryEngine::AudioEvent::Type::PlaySound:
                m_audioEngine->playSound(resolveAsset(event.path), event.volume);
                break;
            case StoryEngine::AudioEvent::Type::PlayVoice:
                m_audioEngine->playVoice(resolveAsset(event.path), event.volume);
                break;
            case StoryEngine::AudioEvent::Type::StopVoice:
                m_audioEngine->stopVoice();
                break;
            case StoryEngine::AudioEvent::Type::PlayAmbient:
                m_audioEngine->playAmbient(resolveAsset(event.path), event.loop, event.fade, event.volume);
                break;
            case StoryEngine::AudioEvent::Type::StopAmbient:
                m_audioEngine->stopAmbient(event.fade);
                break;
        }
    });
    m_storyEngine->setTextFormatter([this](std::string_view text) {
        return m_textRenderer->wrapText(text, m_dialogueBox->getMaxTextWidth(), m_dialogueBox->getTextScale());
    });
    m_storyEngine->setAnimationEventHandler([this](const std::string& charId, CharacterAnimationEvent::Type type, float p1, float p2) {
        auto it = m_characters.find(charId);
        if (it != m_characters.end()) {
            if (type == CharacterAnimationEvent::Type::Shake) {
                it->second->shake(p1, p2);
            } else if (type == CharacterAnimationEvent::Type::Hop) {
                it->second->hop(p1, p2);
            } else if (type == CharacterAnimationEvent::Type::Flash) {
                it->second->flash(p1);
            }
        }
    });

    std::string startScript = Config::getString("start_script", "");
    if (startScript.empty()) {
        startScript = Config::getString("story.start_script", "assets/scripts/demo.vn");
    }
    float textSpeed = Config::getFloat("story.text_speed", 0.020f);
    m_storyEngine->setTextSpeed(textSpeed);
    m_storyEngine->loadScript(resolveAsset(startScript));

    // Подготовка SharedContext и StateManager
    m_context = std::make_unique<SharedContext>();
    m_context->window = m_window.get();
    m_context->spriteRenderer = m_spriteRenderer.get();
    m_context->textRenderer = m_textRenderer.get();
    m_context->renderPipeline = m_renderPipeline.get();
    m_context->audioEngine = m_audioEngine.get();
    m_context->storyEngine = m_storyEngine.get();
    m_context->dialogueBox = m_dialogueBox.get();
    m_context->characters = &m_characters;

    m_stateManager = std::make_unique<StateManager>(*m_context);
    m_context->stateManager = m_stateManager.get();

    m_context->showToast = [this](const std::string& msg) { showToast(msg); };
    m_context->captureScreenThumbnail = [this](std::vector<uint8_t>& p, int& w, int& h) { captureScreenThumbnail(p, w, h); };
    m_context->quit = [this]() { m_isRunning = false; };
    m_context->resolveAsset = resolveAsset;
    m_context->getOrCreateCharacter = [this](const std::string& name) -> CharacterSprite& { return getOrCreateCharacter(name); };
    m_context->applyAudioSettings = [this]() { applyAudioSettings(); };
    m_context->applySettings = [this]() { applySettings(); };
    m_context->restoreGameState = [this](const GameSaveState& state) { return restoreGameState(state); };

    // Стартуем с Главного меню
    m_stateManager->changeState(std::make_unique<MainMenuState>(*m_context));

    m_isRunning = true;
    m_lastFrameTime = static_cast<float>(glfwGetTime());

    std::cout << "[Application] Initialization complete!" << std::endl;
    return true;
}

void Application::applyAudioSettings() {
    if (!m_audioEngine) return;
    m_audioEngine->setMasterVolume(Config::getFloat("audio.master_volume", 1.0f));
    m_audioEngine->setBGMVolume(Config::getFloat("audio.bgm_volume", 0.7f));
    m_audioEngine->setSFXVolume(Config::getFloat("audio.sfx_volume", 0.8f));
    m_audioEngine->setVoiceVolume(Config::getFloat("audio.voice_volume", 0.9f));
    m_audioEngine->setAmbientVolume(Config::getFloat("audio.ambient_volume", 0.6f));
}

void Application::applySettings() {
    applyAudioSettings();
    if (m_window) {
        m_window->setVSync(Config::getBool("window.vsync", true));
        m_window->setFullscreen(Config::getBool("window.fullscreen", false));
    }
    if (m_storyEngine) {
        m_storyEngine->setTextSpeed(Config::getFloat("story.text_speed", 0.030f));
    }
    std::string lang = Config::getString("locale.default", "ru");
    Localization::setLanguage(lang);
}

void Application::showToast(const std::string& message) {
    m_toastMessage = message;
    m_toastTimer = 2.5f;
}

void Application::captureScreenThumbnail(std::vector<uint8_t>& outPixels, int& outW, int& outH) {
    if (m_renderPipeline && m_storyEngine && m_spriteRenderer && m_textRenderer) {
        int fboW = m_renderPipeline->getSceneFBO().getWidth();
        int fboH = m_renderPipeline->getSceneFBO().getHeight();
        outW = fboW;
        outH = fboH;
        outPixels.resize(static_cast<size_t>(fboW * fboH * 3));

        m_renderPipeline->beginScene();

        // 1. Игровой фон
        const std::string& bgPath = m_storyEngine->getCurrentBackground();
        if (!bgPath.empty()) {
            auto bgTex = ResourceManager::loadTexture(bgPath, resolveAsset(bgPath));
            if (bgTex) {
                m_spriteRenderer->drawSprite(*bgTex, glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f));
            } else {
                m_spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.11f, 0.13f, 0.20f, 1.0f));
            }
        }

        // 2. Персонажи
        std::vector<CharacterSprite*> sortedCharacters;
        for (auto& [name, character] : m_characters) {
            if (character->isVisible()) sortedCharacters.push_back(character.get());
        }
        std::stable_sort(sortedCharacters.begin(), sortedCharacters.end(), [](const auto* a, const auto* b) {
            return a->getLayer() < b->getLayer();
        });
        for (auto* character : sortedCharacters) {
            character->render(*m_spriteRenderer, *m_textRenderer);
        }

        // 3. Текст реплики
        if (m_dialogueBox) {
            m_dialogueBox->render(
                *m_spriteRenderer, *m_textRenderer,
                m_storyEngine->getCurrentSpeaker(),
                m_storyEngine->getVisibleText(),
                m_storyEngine->isWaitingForNextClick()
            );
        }

        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, fboW, fboH, GL_RGB, GL_UNSIGNED_BYTE, outPixels.data());
        m_renderPipeline->endScene();
    }
}

bool Application::restoreGameState(const GameSaveState& state) {
    if (!m_storyEngine) return false;
    m_characters.clear();

    if (!m_storyEngine->restoreSaveState(state)) return false;

    // 1. Автоматически регистрируем персонажей
    for (const auto& [id, def] : m_storyEngine->getDefinedCharacters()) {
        auto& sprite = getOrCreateCharacter(id);
        sprite.setDisplayName(def.displayName);
        if (def.hasCustomScale) {
            sprite.setScale(def.scale);
        }
        for (const auto& [expr, path] : def.expressions) {
            if (!path.empty()) {
                sprite.addExpression(expr, resolveAsset(path));
            }
        }
    }

    // 2. Синхронизируем персонажей со StoryEngine
    for (const auto& [name, cd] : state.activeCharacters) {
        auto& sprite = getOrCreateCharacter(name);
        sprite.setExpression(cd.expression);
        if (cd.hasCustomScale) sprite.setScale(cd.scale);
        if (!cd.positionSlot.empty()) {
            sprite.setSlotPosition(cd.positionSlot, false);
        } else {
            sprite.setPosition(glm::vec2(cd.posX, cd.posY), false);
        }
        sprite.setLayer(cd.layer);
        sprite.setTint(cd.tint);
        sprite.setSilhouette(cd.isSilhouette, cd.silhouetteColor);
        sprite.setSepia(cd.sepia);
        sprite.setAlpha(1.0f);
    }

    // 3. Восстанавливаем звук
    if (m_audioEngine) {
        m_audioEngine->setMusicDucked(false);
        if (!state.bgmPath.empty()) {
            if (state.bgmPath != m_audioEngine->getCurrentMusicPath() || !m_audioEngine->isMusicPlaying()) {
                m_audioEngine->playMusic(resolveAsset(state.bgmPath), state.bgmLoop, 0.5f, state.bgmVolume);
            }
        } else {
            m_audioEngine->stopMusic(0.5f);
        }

        if (!state.ambientPath.empty()) {
            if (state.ambientPath != m_audioEngine->getCurrentAmbientPath() || !m_audioEngine->isAmbientPlaying()) {
                m_audioEngine->playAmbient(resolveAsset(state.ambientPath), state.ambientLoop, 0.5f, state.ambientVolume);
            }
        } else {
            m_audioEngine->stopAmbient(0.5f);
        }
    }

    return true;
}

void Application::run() {
    if (!init()) {
        throw std::runtime_error("Application initialization failed");
    }

    while (m_isRunning && !m_window->shouldClose()) {
        float currentFrameTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrameTime - m_lastFrameTime;
        m_lastFrameTime = currentFrameTime;

        if (deltaTime > 0.1f) deltaTime = 0.1f;

        Input::update();
        if (m_window) {
            m_window->pollEvents();
        }
        processInput();
        update(deltaTime);
        render();
    }
}

void Application::processInput() {
    if (m_stateManager) {
        m_stateManager->handleInput();
    }
}

void Application::update(float deltaTime) {
    if (m_toastTimer > 0.0f) {
        m_toastTimer -= deltaTime;
        if (m_toastTimer <= 0.0f) {
            m_toastTimer = 0.0f;
            m_toastMessage.clear();
        }
    }

    if (m_audioEngine) {
        m_audioEngine->update(deltaTime);
    }

    if (m_renderPipeline) {
        m_renderPipeline->updateTransition(deltaTime);
    }

    if (m_stateManager) {
        m_stateManager->update(deltaTime);
    }
}

void Application::render() {
    glClearColor(0.04f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_stateManager) {
        m_stateManager->render();
    }

    // Рендеринг всплывающего Toast-уведомления поверх всех экранов
    if (m_toastTimer > 0.0f && !m_toastMessage.empty() && m_spriteRenderer && m_textRenderer) {
        float scale = 0.65f;
        glm::vec2 textSize = m_textRenderer->measureText(m_toastMessage, scale);
        float padX = 24.0f, padH = 12.0f;
        float boxW = textSize.x + padX * 2.0f;
        float boxH = textSize.y + padH * 2.0f;
        float boxX = (1280.0f - boxW) * 0.5f;
        float boxY = 645.0f;

        float alpha = std::clamp(m_toastTimer / 0.4f, 0.0f, 1.0f);
        m_spriteRenderer->drawRect(glm::vec2(boxX, boxY), glm::vec2(boxW, boxH), glm::vec4(0.08f, 0.10f, 0.16f, 0.92f * alpha));

        float bw = 1.5f;
        glm::vec4 borderCol(0.35f, 0.65f, 0.95f, 0.85f * alpha);
        m_spriteRenderer->drawRect(glm::vec2(boxX, boxY), glm::vec2(boxW, bw), borderCol);
        m_spriteRenderer->drawRect(glm::vec2(boxX, boxY + boxH - bw), glm::vec2(boxW, bw), borderCol);
        m_spriteRenderer->drawRect(glm::vec2(boxX, boxY), glm::vec2(bw, boxH), borderCol);
        m_spriteRenderer->drawRect(glm::vec2(boxX + boxW - bw, boxY), glm::vec2(bw, boxH), borderCol);

        m_textRenderer->renderText(m_toastMessage, boxX + padX, boxY + padH + 2.0f, scale, glm::vec4(1.0f, 1.0f, 1.0f, alpha));
    }

    if (m_window) {
        m_window->swapBuffers();
    }
}

void Application::shutdown() {
    m_characters.clear();
    m_stateManager.reset();
    m_context.reset();

    Localization::clear();
    m_storyEngine.reset();
    m_textRenderer.reset();
    m_spriteRenderer.reset();
    ResourceManager::clear();

    m_pythonEngine.reset();
    m_renderPipeline.reset();
    m_window.reset();
}
