#include "Application.hpp"

#include "core/Window.hpp"
#include "core/Input.hpp"
#include "core/Config.hpp"
#include "renderer/Shader.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"
#include "resource/ResourceManager.hpp"
#include "resource/Localization.hpp"
#include "scene/CharacterSprite.hpp"
#include "renderer/RenderPipeline.hpp"
#include "audio/AudioEngine.hpp"
#include "scene/Scene.hpp"
#include "scripting_api/PythonEngine.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

// Умный поиск ассетов: ищет файл в текущей папке и в родительской (для запуска из CLion cmake-build-debug)
static std::string resolveAsset(const std::string& relativePath) {
    if (relativePath.empty()) return "";
    if (std::filesystem::exists(relativePath)) {
        return relativePath;
    }
    if (std::filesystem::exists("../" + relativePath)) {
        return "../" + relativePath;
    }
    return relativePath;
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
        std::string langCode = key.substr(7); // отсекаем префикс "locale."
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
        float masterVol = Config::getFloat("audio.master_volume", 1.0f);
        float bgmVol = Config::getFloat("audio.bgm_volume", 0.8f);
        float sfxVol = Config::getFloat("audio.sfx_volume", 1.0f);
        float voiceVol = Config::getFloat("audio.voice_volume", 1.0f);
        float ambientVol = Config::getFloat("audio.ambient_volume", 0.8f);
        m_audioEngine->setMasterVolume(masterVol);
        m_audioEngine->setBGMVolume(bgmVol);
        m_audioEngine->setSFXVolume(sfxVol);
        m_audioEngine->setVoiceVolume(voiceVol);
        m_audioEngine->setAmbientVolume(ambientVol);

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

    // Инициализируем StoryEngine и загружаем стартовый сценарий из config.vn
    m_storyEngine = std::make_unique<StoryEngine>();
    m_storyEngine->setAssetResolver([](const std::string& path) {
        return resolveAsset(path);
    });
    m_storyEngine->setScriptChangeHandler([this](const std::string& newScript) {
        m_characters.clear();
        m_choiceButtons.clear();
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

    // Инициализируем кнопки меню на основе текущей локализации
    initUI();

    m_isRunning = true;
    m_lastFrameTime = static_cast<float>(glfwGetTime());
    return true;
}

void Application::initUI() {
    // -------------------------------------------------------------
    // Кнопки Главного Меню (тексты берутся из файлов локализации)
    // -------------------------------------------------------------
    m_mainMenuButtons.clear();

    m_mainMenuButtons.emplace_back(
        glm::vec2(100.0f, 300.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.new_game"),
        [this]() {
            std::string startScript = Config::getString("start_script", "");
            if (startScript.empty()) {
                startScript = Config::getString("story.start_script", "assets/scripts/demo.vn");
            }
            m_characters.clear();
            m_choiceButtons.clear();
            if (m_audioEngine) m_audioEngine->setMusicDucked(false);
            m_storyEngine->loadScript(resolveAsset(startScript));
            m_storyEngine->executeNextCommand();
            m_gameState = GameState::Gameplay;
        }
    );

    m_mainMenuButtons.emplace_back(
        glm::vec2(100.0f, 370.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.load"),
        [this]() {
            openLoadMenu(GameState::MainMenu);
        }
    );

    m_mainMenuButtons.emplace_back(
        glm::vec2(100.0f, 440.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.settings"),
        [this]() {
            openSettingsMenu(GameState::MainMenu);
        }
    );

    m_mainMenuButtons.emplace_back(
        glm::vec2(100.0f, 510.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.exit"),
        [this]() {
            m_isRunning = false;
        }
    );

    // -------------------------------------------------------------
    // Кнопки Меню Паузы (6 кнопок: продолжить, сохранить, загрузить, настройки, главное меню, выход)
    // -------------------------------------------------------------
    m_pauseMenuButtons.clear();

    m_pauseMenuButtons.emplace_back(
        glm::vec2(490.0f, 175.0f), glm::vec2(300.0f, 42.0f),
        LOC("pause.resume"),
        [this]() {
            m_gameState = GameState::Gameplay;
            if (m_audioEngine) m_audioEngine->setMusicDucked(false);
        }
    );

    m_pauseMenuButtons.emplace_back(
        glm::vec2(490.0f, 227.0f), glm::vec2(300.0f, 42.0f),
        LOC("pause.save"),
        [this]() {
            openSaveMenu();
        }
    );

    m_pauseMenuButtons.emplace_back(
        glm::vec2(490.0f, 279.0f), glm::vec2(300.0f, 42.0f),
        LOC("pause.load"),
        [this]() {
            openLoadMenu(GameState::PauseMenu);
        }
    );

    m_pauseMenuButtons.emplace_back(
        glm::vec2(490.0f, 331.0f), glm::vec2(300.0f, 42.0f),
        LOC("pause.settings"),
        [this]() {
            openSettingsMenu(GameState::PauseMenu);
        }
    );

    m_pauseMenuButtons.emplace_back(
        glm::vec2(490.0f, 383.0f), glm::vec2(300.0f, 42.0f),
        LOC("pause.main_menu"),
        [this]() {
            m_gameState = GameState::MainMenu;
            if (m_audioEngine) {
                m_audioEngine->setMusicDucked(false);
                m_audioEngine->stopAmbient(1.0f);
                std::string menuMusic = Config::getString("menu.music", "");
                if (!menuMusic.empty()) {
                    m_audioEngine->playMusic(resolveAsset(menuMusic), true, m_audioEngine->getDefaultBGMFade());
                }
            }
        }
    );

    m_pauseMenuButtons.emplace_back(
        glm::vec2(490.0f, 435.0f), glm::vec2(300.0f, 42.0f),
        LOC("pause.exit"),
        [this]() {
            m_isRunning = false;
        }
    );

    // -------------------------------------------------------------
    // Кнопка закрытия окна Беклога (История диалогов)
    // -------------------------------------------------------------
    m_backlogCloseButton = std::make_unique<Button>(
        glm::vec2(1050.0f, 14.0f), glm::vec2(110.0f, 44.0f),
        LOC("backlog.close"),
        [this]() {
            m_gameState = GameState::Gameplay;
            if (m_audioEngine) m_audioEngine->setMusicDucked(false);
        }
    );

    // -------------------------------------------------------------
    // Кнопка «Назад» в меню сохранений и загрузки
    // -------------------------------------------------------------
    m_saveLoadBackButton = std::make_unique<Button>(
        glm::vec2(1050.0f, 14.0f), glm::vec2(110.0f, 44.0f),
        LOC("saveload.back"),
        [this]() {
            m_gameState = m_saveLoadReturnState;
            if (m_gameState == GameState::Gameplay && m_audioEngine) {
                m_audioEngine->setMusicDucked(false);
            }
        }
    );

    // -------------------------------------------------------------
    // Кнопки Меню настроек
    // -------------------------------------------------------------
    m_settingsBackButton = std::make_unique<Button>(
        glm::vec2(1070.0f, 14.0f), glm::vec2(130.0f, 44.0f),
        LOC("settings.back"),
        [this]() {
            m_gameState = m_settingsReturnState;
            Config::saveUserSettings();
            showToast(LOC("settings.saved_toast"));
        }
    );

    m_settingsResetButton = std::make_unique<Button>(
        glm::vec2(925.0f, 14.0f), glm::vec2(130.0f, 44.0f),
        LOC("settings.defaults"),
        [this]() {
            Config::resetUserSettings();
            applySettings();
            initUI();
            showToast(LOC("settings.reset_toast"));
        }
    );
}

void Application::updateChoiceButtons() {
    m_choiceButtons.clear();
    const auto& choices = m_storyEngine->getCurrentChoices();
    if (choices.empty()) return;

    size_t count = choices.size();
    float btnW = 640.0f;
    float btnH = 50.0f;
    float btnSpacing = 14.0f;
    float winW = 700.0f;

    // Высота окна динамически подстраивается под количество вариантов
    // Заголовок (18px отступ + текст + 18px разделитель) = 68px
    // Кнопки: count * btnH + (count - 1) * btnSpacing
    // Нижний отступ: 18px
    float headerH = 68.0f;
    float bottomPad = 18.0f;
    float winH = headerH + static_cast<float>(count) * btnH + (count > 0 ? static_cast<float>(count - 1) * btnSpacing : 0.0f) + bottomPad;

    float winX = (1280.0f - winW) * 0.5f;
    // Центрируем окно по вертикали в зоне над диалоговым окном
    float winY = std::max(20.0f, 250.0f - winH * 0.5f);

    m_choiceWindowPos = glm::vec2(winX, winY);
    m_choiceWindowSize = glm::vec2(winW, winH);

    float buttonsStartY = winY + headerH;
    float btnX = winX + (winW - btnW) * 0.5f;

    for (size_t i = 0; i < count; ++i) {
        float btnY = buttonsStartY + static_cast<float>(i) * (btnH + btnSpacing);
        m_choiceButtons.emplace_back(
            glm::vec2(btnX, btnY), glm::vec2(btnW, btnH),
            choices[i].text,
            [this, i]() {
                m_storyEngine->chooseOption(i);
                m_choiceButtons.clear();
            }
        );
    }
}

void Application::run() {
    if (!init()) {
        throw std::runtime_error("Application initialization failed");
    }

    while (m_isRunning && !m_window->shouldClose()) {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;

        processInput(deltaTime);
        update(deltaTime);
        render();

        m_window->swapBuffers();
    }
}

void Application::processInput(float deltaTime) {
    Input::update();
    m_window->pollEvents();

    glm::vec2 mousePos = Input::getMousePosition();
    bool mouseClicked = Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);

    switch (m_gameState) {
        case GameState::MainMenu: {
            for (auto& btn : m_mainMenuButtons) {
                btn.update(mousePos, mouseClicked);
            }
            break;
        }
        case GameState::Gameplay: {
            // Быстрое сохранение (F5) и быстрая загрузка (F9)
            if (Input::isKeyJustPressed(GLFW_KEY_F5)) {
                performSave(SaveManager::QUICKSAVE_SLOT_INDEX);
                break;
            }
            if (Input::isKeyJustPressed(GLFW_KEY_F9)) {
                if (SaveManager::doesSlotExist(SaveManager::QUICKSAVE_SLOT_INDEX)) {
                    performLoad(SaveManager::QUICKSAVE_SLOT_INDEX);
                }
                break;
            }

            // Открытие паузы по Escape
            if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE)) {
                m_gameState = GameState::PauseMenu;
                if (m_audioEngine) m_audioEngine->setMusicDucked(true);
                break;
            }

            // Открытие истории диалогов (Беклог) по колесу мыши вверх или клавишам H / L
            float scrollDelta = Input::getMouseScrollY();
            if (scrollDelta > 0.1f || Input::isKeyJustPressed(GLFW_KEY_H) || Input::isKeyJustPressed(GLFW_KEY_L)) {
                m_gameState = GameState::Backlog;
                if (m_audioEngine) m_audioEngine->setMusicDucked(true);
                m_backlogScrollY = 999999.0f; // На старте скроллим в самый низ к последним репликам
                m_isDraggingScrollbar = false;
                break;
            }

            // Если игра ожидает интерактивный выбор развилки
            if (m_storyEngine->isWaitingForChoice()) {
                if (m_choiceButtons.empty()) {
                    updateChoiceButtons();
                }
                for (auto& btn : m_choiceButtons) {
                    btn.update(mousePos, mouseClicked);
                }
                break;
            }

            // Перелистывание диалогов по пробелу или клику мыши (QoL: первый клик мгновенно завершает печать)
            if (Input::isKeyJustPressed(GLFW_KEY_SPACE) || mouseClicked) {
                if (!m_storyEngine->isTextFullyRevealed()) {
                    m_storyEngine->revealFullText();
                } else {
                    m_storyEngine->nextStep();
                    if (m_storyEngine->isWaitingForChoice()) {
                        updateChoiceButtons();
                    }
                }
            }
            break;
        }
        case GameState::PauseMenu: {
            // Закрытие паузы по Escape
            if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE)) {
                m_gameState = GameState::Gameplay;
                if (m_audioEngine) m_audioEngine->setMusicDucked(false);
                break;
            }

            for (auto& btn : m_pauseMenuButtons) {
                btn.update(mousePos, mouseClicked);
            }
            break;
        }
        case GameState::Backlog: {
            // Закрытие беклога по Escape, правой кнопке мыши, H или L
            if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE) ||
                Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT) ||
                Input::isKeyJustPressed(GLFW_KEY_H) ||
                Input::isKeyJustPressed(GLFW_KEY_L)) {
                m_gameState = GameState::Gameplay;
                if (m_audioEngine) m_audioEngine->setMusicDucked(false);
                break;
            }

            // Кнопка "Закрыть" в заголовке беклога
            if (m_backlogCloseButton) {
                m_backlogCloseButton->update(mousePos, mouseClicked);
                if (m_gameState != GameState::Backlog) {
                    break;
                }
            }

            // Прокрутка колесом мыши
            float scrollDelta = Input::getMouseScrollY();
            if (std::abs(scrollDelta) > 0.01f) {
                // Колесико вверх (scrollDelta > 0) -> уменьшаем scrollY (движемся назад во времени)
                // Колесико вниз (scrollDelta < 0) -> увеличиваем scrollY (к новым репликам)
                if (scrollDelta < -0.1f && m_backlogScrollY >= m_backlogMaxScrollY - 0.5f) {
                    // QoL для ВН: прокрутка вниз в самом конце списка закрывает беклог обратно в игру
                    m_gameState = GameState::Gameplay;
                    if (m_audioEngine) m_audioEngine->setMusicDucked(false);
                    break;
                }
                m_backlogScrollY -= scrollDelta * 60.0f;
            }

            // Навигация с клавиатуры
            if (Input::isKeyJustPressed(GLFW_KEY_UP)) {
                m_backlogScrollY -= 60.0f;
            }
            if (Input::isKeyJustPressed(GLFW_KEY_DOWN)) {
                if (m_backlogScrollY >= m_backlogMaxScrollY - 0.5f) {
                    m_gameState = GameState::Gameplay;
                    if (m_audioEngine) m_audioEngine->setMusicDucked(false);
                    break;
                }
                m_backlogScrollY += 60.0f;
            }
            if (Input::isKeyJustPressed(GLFW_KEY_PAGE_UP)) {
                m_backlogScrollY -= 300.0f;
            }
            if (Input::isKeyJustPressed(GLFW_KEY_PAGE_DOWN)) {
                m_backlogScrollY += 300.0f;
            }
            if (Input::isKeyJustPressed(GLFW_KEY_HOME)) {
                m_backlogScrollY = 0.0f;
            }
            if (Input::isKeyJustPressed(GLFW_KEY_END)) {
                m_backlogScrollY = m_backlogMaxScrollY;
            }

            // Интерактивный скроллбар
            float trackX = 1155.0f;
            float trackY = 78.0f;
            float trackW = 12.0f;
            float trackH = 618.0f;

            if (Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                if (mousePos.x >= trackX - 8.0f && mousePos.x <= trackX + trackW + 8.0f &&
                    mousePos.y >= trackY && mousePos.y <= trackY + trackH) {
                    m_isDraggingScrollbar = true;
                    if (trackH > 40.0f && m_backlogMaxScrollY > 0.0f) {
                        float thumbH = std::max(40.0f, trackH * (trackH / (trackH + m_backlogMaxScrollY)));
                        float travel = trackH - thumbH;
                        if (travel > 1.0f) {
                            float targetY = std::clamp(mousePos.y - trackY - thumbH * 0.5f, 0.0f, travel);
                            m_backlogScrollY = (targetY / travel) * m_backlogMaxScrollY;
                        }
                    }
                    m_scrollbarDragStartMouseY = mousePos.y;
                    m_scrollbarDragStartScrollY = m_backlogScrollY;
                }
            }
            if (Input::isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
                m_isDraggingScrollbar = false;
            }
            if (m_isDraggingScrollbar) {
                float deltaMouseY = mousePos.y - m_scrollbarDragStartMouseY;
                if (trackH > 40.0f && m_backlogMaxScrollY > 0.0f) {
                    float thumbH = std::max(40.0f, trackH * (trackH / (trackH + m_backlogMaxScrollY)));
                    float travel = trackH - thumbH;
                    if (travel > 1.0f) {
                        m_backlogScrollY = m_scrollbarDragStartScrollY + (deltaMouseY / travel) * m_backlogMaxScrollY;
                    }
                }
            }

            m_backlogScrollY = std::clamp(m_backlogScrollY, 0.0f, std::max(0.0f, m_backlogMaxScrollY));
            break;
        }
        case GameState::SaveMenu:
        case GameState::LoadMenu: {
            bool isSaving = (m_gameState == GameState::SaveMenu);

            // Закрытие по Escape или ПКМ -> возврат в исходное меню
            if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE) || Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
                m_gameState = m_saveLoadReturnState;
                if (m_gameState == GameState::Gameplay && m_audioEngine) {
                    m_audioEngine->setMusicDucked(false);
                }
                break;
            }

            if (m_saveLoadBackButton) {
                m_saveLoadBackButton->update(mousePos, mouseClicked);
                if (m_gameState != GameState::SaveMenu && m_gameState != GameState::LoadMenu) {
                    break;
                }
            }

            // Обработка клика по карточкам слотов (Слоты 1..6)
            float startX = 80.0f;
            float startY = 88.0f;
            float cardW = 350.0f;
            float cardH = 270.0f;
            float spacingX = 35.0f;
            float spacingY = 24.0f;

            if (mouseClicked) {
                for (int slot = 1; slot <= 6; ++slot) {
                    int col = (slot - 1) % 3;
                    int row = (slot - 1) / 3;
                    float x = startX + static_cast<float>(col) * (cardW + spacingX);
                    float y = startY + static_cast<float>(row) * (cardH + spacingY);

                    // Клик по кнопке удаления (✕)
                    if (mousePos.x >= x + cardW - 38.0f && mousePos.x <= x + cardW - 10.0f &&
                        mousePos.y >= y + 8.0f && mousePos.y <= y + 36.0f) {
                        if (SaveManager::doesSlotExist(slot)) {
                            SaveManager::deleteSlot(slot);
                            refreshSaveSlotsMetadata();
                            showToast(LOC("saveload.deleted_toast"));
                            break;
                        }
                    }

                    // Клик по карточке слота
                    if (mousePos.x >= x && mousePos.x <= x + cardW &&
                        mousePos.y >= y && mousePos.y <= y + cardH) {
                        if (isSaving) {
                            performSave(slot);
                        } else {
                            if (SaveManager::doesSlotExist(slot)) {
                                performLoad(slot);
                            }
                        }
                        break;
                    }
                }
            }
            break;
        }
        case GameState::SettingsMenu: {
            // 1. Закрытие по Escape или ПКМ -> сохранение и возврат
            if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE) || Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
                m_gameState = m_settingsReturnState;
                Config::saveUserSettings();
                showToast(LOC("settings.saved_toast"));
                break;
            }

            // 2. Кнопка "Назад"
            if (m_settingsBackButton) {
                m_settingsBackButton->update(mousePos, mouseClicked);
                if (m_gameState != GameState::SettingsMenu) break;
            }

            // 3. Кнопка "Сброс"
            if (m_settingsResetButton) {
                m_settingsResetButton->update(mousePos, mouseClicked);
            }

            bool isLeftPressed = Input::isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);

            float sliderStartY = 145.0f;
            float sliderStepY = 85.0f;
            float audioTrackX = 115.0f;
            float audioTrackW = 490.0f;

            // Начало драга слайдеров
            if (mouseClicked && m_activeDraggingSlider == -1) {
                for (int i = 0; i < 5; ++i) {
                    float y = sliderStartY + static_cast<float>(i) * sliderStepY;
                    float trackY = y + 28.0f;
                    if (mousePos.x >= audioTrackX - 10.0f && mousePos.x <= audioTrackX + audioTrackW + 10.0f &&
                        mousePos.y >= trackY - 14.0f && mousePos.y <= trackY + 22.0f) {
                        m_activeDraggingSlider = i;
                        break;
                    }
                }

                if (m_activeDraggingSlider == -1) {
                    float textTrackX = 675.0f;
                    float textTrackW = 480.0f;
                    float textTrackY = 145.0f + 28.0f;
                    if (mousePos.x >= textTrackX - 10.0f && mousePos.x <= textTrackX + textTrackW + 10.0f &&
                        mousePos.y >= textTrackY - 14.0f && mousePos.y <= textTrackY + 22.0f) {
                        m_activeDraggingSlider = 5;
                    }
                }
            }

            // Перетаскивание слайдера
            if (isLeftPressed && m_activeDraggingSlider != -1) {
                if (m_activeDraggingSlider >= 0 && m_activeDraggingSlider <= 4) {
                    float t = (mousePos.x - audioTrackX) / audioTrackW;
                    t = std::clamp(t, 0.0f, 1.0f);
                    t = std::round(t * 100.0f) / 100.0f;

                    std::string keys[5] = {
                        "audio.master_volume", "audio.bgm_volume", "audio.sfx_volume",
                        "audio.voice_volume", "audio.ambient_volume"
                    };
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(2) << t;
                    Config::setUserSetting(keys[m_activeDraggingSlider], ss.str());
                    applyAudioSettings();
                } else if (m_activeDraggingSlider == 5) {
                    float textTrackX = 675.0f;
                    float textTrackW = 480.0f;
                    float t = (mousePos.x - textTrackX) / textTrackW;
                    t = std::clamp(t, 0.0f, 1.0f);
                    float speed = 0.060f - t * (0.060f - 0.005f);
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(3) << speed;
                    Config::setUserSetting("story.text_speed", ss.str());
                    if (m_storyEngine) m_storyEngine->setTextSpeed(speed);
                }
            }

            // Отпускание кнопки мыши
            if (!isLeftPressed && m_activeDraggingSlider != -1) {
                m_activeDraggingSlider = -1;
                Config::saveUserSettings();
            }

            // Клики по кнопкам языков и тумблерам
            if (mouseClicked) {
                // Кнопки языков
                auto langs = Localization::getAvailableLanguages();
                if (langs.empty()) langs = {"ru", "en", "uk"};
                float langBtnX = 675.0f;
                float langBtnY = 235.0f + 26.0f;
                float langBtnW = 145.0f;
                float langBtnH = 36.0f;
                float langBtnSpacing = 16.0f;

                for (size_t i = 0; i < langs.size(); ++i) {
                    float x = langBtnX + static_cast<float>(i) * (langBtnW + langBtnSpacing);
                    if (mousePos.x >= x && mousePos.x <= x + langBtnW &&
                        mousePos.y >= langBtnY && mousePos.y <= langBtnY + langBtnH) {
                        Localization::setLanguage(langs[i]);
                        Config::setUserSetting("locale.default", langs[i]);
                        Config::saveUserSettings();
                        initUI();
                        break;
                    }
                }

                // Тумблер VSync
                if (mousePos.x >= 1045.0f && mousePos.x <= 1145.0f &&
                    mousePos.y >= 340.0f && mousePos.y <= 374.0f) {
                    bool cur = Config::getBool("window.vsync", true);
                    bool next = !cur;
                    Config::setUserSetting("window.vsync", next ? "true" : "false");
                    if (m_window) m_window->setVSync(next);
                    Config::saveUserSettings();
                }

                // Тумблер Полноэкранный режим
                if (mousePos.x >= 1045.0f && mousePos.x <= 1145.0f &&
                    mousePos.y >= 420.0f && mousePos.y <= 454.0f) {
                    bool cur = m_window ? m_window->isFullscreen() : Config::getBool("window.fullscreen", false);
                    bool next = !cur;
                    Config::setUserSetting("window.fullscreen", next ? "true" : "false");
                    if (m_window) m_window->setFullscreen(next);
                    Config::saveUserSettings();
                }

                // Тумблер Приглушать неактивных
                if (mousePos.x >= 1045.0f && mousePos.x <= 1145.0f &&
                    mousePos.y >= 500.0f && mousePos.y <= 534.0f) {
                    bool cur = Config::getBool("story.auto_dim_inactive", false);
                    bool next = !cur;
                    Config::setUserSetting("story.auto_dim_inactive", next ? "true" : "false");
                    Config::saveUserSettings();
                }
            }
            break;
        }
    }
}

void Application::update(float deltaTime) {
    if (m_toastTimer > 0.0f) {
        m_toastTimer -= deltaTime;
    }

    if (m_renderPipeline) {
        m_renderPipeline->updateTransition(deltaTime);
    }

    if (m_audioEngine) {
        m_audioEngine->update(deltaTime);
    }

    if (m_gameState == GameState::Gameplay) {
        m_storyEngine->update(deltaTime);
        if (m_dialogueBox) {
            m_dialogueBox->update(deltaTime);
        }
        if (m_storyEngine->isWaitingForChoice() && m_choiceButtons.empty()) {
            updateChoiceButtons();
        }

        // 1. Автоматически регистрируем персонажей, определенных в сценарии (define)
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

        // 2. Синхронизируем состояние активных персонажей со StoryEngine (координаты, слой, видимость)
        const auto& activeCharacters = m_storyEngine->getActiveCharacters();
        for (auto& [name, character] : m_characters) {
            auto it = activeCharacters.find(name);
            if (it != activeCharacters.end()) {
                if (it->second.hasCustomScale) {
                    character->setScale(it->second.scale);
                }
                if (!it->second.positionSlot.empty()) {
                    character->setSlotPosition(it->second.positionSlot);
                } else {
                    character->setPosition(it->second.position);
                }
                character->setExpression(it->second.expression);
                character->setLayer(it->second.layer);
                character->setTint(it->second.tint);
                character->setSilhouette(it->second.isSilhouette, it->second.silhouetteColor);
                character->setSepia(it->second.sepia);

                // Опциональное приглушение не говорящих в данный момент персонажей
                if (Config::getBool("story.auto_dim_inactive", false) && !m_storyEngine->getCurrentSpeaker().empty()) {
                    bool isSpeaker = (character->getDisplayName() == m_storyEngine->getCurrentSpeaker() || name == m_storyEngine->getCurrentSpeaker());
                    if (!isSpeaker) {
                        character->setTint(it->second.tint * glm::vec4(0.72f, 0.72f, 0.76f, 1.0f));
                    }
                }

                character->fadeIn();
            } else {
                character->fadeOut();
            }
            character->update(deltaTime);
        }
    }
}

void Application::render() {
    int fbWidth = 1280, fbHeight = 720;
    if (m_window) {
        m_window->getFramebufferSize(&fbWidth, &fbHeight);
    }

    switch (m_gameState) {
        case GameState::MainMenu: {
            glViewport(0, 0, fbWidth, fbHeight);
            glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            renderMainMenu();
            break;
        }

        case GameState::Gameplay: {
            if (m_renderPipeline) {
                // 1. Отрисовка всей игры во внеэкранный буфер кадра (FBO)
                m_renderPipeline->beginScene();
                renderGameplay();
                m_renderPipeline->endScene();

                // 2. Вывод кадра на экран через шейдер пост-процессинга с активными эффектами сценария
                PostProcessSettings settings = m_storyEngine ? m_storyEngine->getPostProcessSettings() : PostProcessSettings{};
                m_renderPipeline->renderToScreen(settings, fbWidth, fbHeight);
            } else {
                glViewport(0, 0, fbWidth, fbHeight);
                glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                renderGameplay();
            }
            break;
        }

        case GameState::PauseMenu: {
            if (m_renderPipeline) {
                // 1. Рендерим застывшую сцену игры в FBO
                m_renderPipeline->beginScene();
                renderGameplay();
                m_renderPipeline->endScene();

                // 2. Накладываем эффект мягкого киноразмытия (Blur on Pause) и виньетку
                PostProcessSettings pauseSettings = m_storyEngine ? m_storyEngine->getPostProcessSettings() : PostProcessSettings{};
                pauseSettings.blurStrength = std::max(pauseSettings.blurStrength, 1.6f);
                pauseSettings.vignetteIntensity = std::max(pauseSettings.vignetteIntensity, 0.45f);
                m_renderPipeline->renderToScreen(pauseSettings, fbWidth, fbHeight);

                // 3. Поверх размытого фона рисуем карточку меню паузы
                glViewport(0, 0, fbWidth, fbHeight);
                renderPauseMenu();
            } else {
                glViewport(0, 0, fbWidth, fbHeight);
                glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                renderGameplay();
                renderPauseMenu();
            }
            break;
        }

        case GameState::Backlog: {
            if (m_renderPipeline) {
                // 1. Рендерим застывшую сцену игры в FBO
                m_renderPipeline->beginScene();
                renderGameplay();
                m_renderPipeline->endScene();

                // 2. Накладываем эффект мягкого киноразмытия и виньетку
                PostProcessSettings backlogSettings = m_storyEngine ? m_storyEngine->getPostProcessSettings() : PostProcessSettings{};
                backlogSettings.blurStrength = std::max(backlogSettings.blurStrength, 1.2f);
                backlogSettings.vignetteIntensity = std::max(backlogSettings.vignetteIntensity, 0.40f);
                m_renderPipeline->renderToScreen(backlogSettings, fbWidth, fbHeight);

                // 3. Поверх размытого фона рисуем окно истории диалогов
                glViewport(0, 0, fbWidth, fbHeight);
                renderBacklog();
            } else {
                glViewport(0, 0, fbWidth, fbHeight);
                glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                renderGameplay();
                renderBacklog();
            }
            break;
        }

        case GameState::SaveMenu:
        case GameState::LoadMenu: {
            if (m_renderPipeline) {
                m_renderPipeline->beginScene();
                if (m_saveLoadReturnState == GameState::MainMenu) {
                    renderMainMenu();
                } else {
                    renderGameplay();
                }
                m_renderPipeline->endScene();

                PostProcessSettings settings = m_storyEngine ? m_storyEngine->getPostProcessSettings() : PostProcessSettings{};
                settings.blurStrength = std::max(settings.blurStrength, 1.4f);
                settings.vignetteIntensity = std::max(settings.vignetteIntensity, 0.45f);
                m_renderPipeline->renderToScreen(settings, fbWidth, fbHeight);

                glViewport(0, 0, fbWidth, fbHeight);
                renderSaveLoadMenu(m_gameState == GameState::SaveMenu);
            } else {
                glViewport(0, 0, fbWidth, fbHeight);
                glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                if (m_saveLoadReturnState == GameState::MainMenu) {
                    renderMainMenu();
                } else {
                    renderGameplay();
                }
                renderSaveLoadMenu(m_gameState == GameState::SaveMenu);
            }
            break;
        }

        case GameState::SettingsMenu: {
            if (m_renderPipeline) {
                m_renderPipeline->beginScene();
                if (m_settingsReturnState == GameState::MainMenu) {
                    renderMainMenu();
                } else {
                    renderGameplay();
                }
                m_renderPipeline->endScene();

                PostProcessSettings settings = m_storyEngine ? m_storyEngine->getPostProcessSettings() : PostProcessSettings{};
                settings.blurStrength = std::max(settings.blurStrength, 1.4f);
                settings.vignetteIntensity = std::max(settings.vignetteIntensity, 0.45f);
                m_renderPipeline->renderToScreen(settings, fbWidth, fbHeight);

                glViewport(0, 0, fbWidth, fbHeight);
                renderSettingsMenu();
            } else {
                glViewport(0, 0, fbWidth, fbHeight);
                glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                if (m_settingsReturnState == GameState::MainMenu) {
                    renderMainMenu();
                } else {
                    renderGameplay();
                }
                renderSettingsMenu();
            }
            break;
        }
    }

    // Всплывающее экранное уведомление (Toast)
    if (m_toastTimer > 0.0f && !m_toastMessage.empty()) {
        float alpha = std::clamp(m_toastTimer * 2.0f, 0.0f, 1.0f);
        float textW = m_textRenderer->measureText(m_toastMessage, 0.70f).x;
        float pillW = textW + 48.0f;
        float pillH = 42.0f;
        float pillX = (1280.0f - pillW) * 0.5f;
        float pillY = 650.0f;

        m_spriteRenderer->drawRect(glm::vec2(pillX, pillY), glm::vec2(pillW, pillH), glm::vec4(0.06f, 0.08f, 0.12f, 0.92f * alpha));
        m_spriteRenderer->drawRect(glm::vec2(pillX, pillY), glm::vec2(pillW, 2.0f), glm::vec4(0.25f, 0.85f, 0.45f, 0.95f * alpha));
        m_textRenderer->renderText(m_toastMessage, pillX + 24.0f, pillY + 11.0f, 0.70f, glm::vec4(1.0f, 1.0f, 1.0f, alpha));
    }
}

void Application::renderMainMenu() {
    if (!m_spriteRenderer || !m_textRenderer) return;

    // 1. Фоновое изображение главного меню
    std::string menuBgPath = Config::getString("menu.background", "assets/textures/backgrounds/classroom.jpg");
    auto menuBg = ResourceManager::loadTexture("menu_bg", resolveAsset(menuBgPath));
    if (menuBg) {
        m_spriteRenderer->drawSprite(*menuBg, glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), 0.0f, glm::vec4(0.45f, 0.50f, 0.65f, 1.0f));
    }

    // 2. Декоративная боковая панель меню
    m_spriteRenderer->drawRect(glm::vec2(60.0f, 0.0f), glm::vec2(360.0f, 720.0f), glm::vec4(0.05f, 0.06f, 0.10f, 0.88f));
    m_spriteRenderer->drawRect(glm::vec2(420.0f, 0.0f), glm::vec2(3.0f, 720.0f), glm::vec4(0.35f, 0.55f, 0.95f, 0.6f));

    // 3. Название игры и подзаголовок из локализации
    m_textRenderer->renderText(LOC("menu.title"), 100.0f, 130.0f, 1.5f, glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));
    m_textRenderer->renderText(LOC("menu.subtitle"), 100.0f, 195.0f, 0.75f, glm::vec4(0.45f, 0.65f, 1.0f, 0.9f));

    // 4. Отрисовка кнопок меню
    for (auto& btn : m_mainMenuButtons) {
        btn.render(*m_spriteRenderer, *m_textRenderer);
    }

    // 5. Версия из локализации в нижнем углу
    m_textRenderer->renderText(LOC("menu.version"), 950.0f, 680.0f, 0.65f, glm::vec4(0.4f, 0.45f, 0.55f, 0.8f));
}

void Application::renderGameplay() {
    if (!m_spriteRenderer || !m_textRenderer || !m_storyEngine) return;

    // 1. Игровой фон (загружаем изображение, заданное в сценарии scene "...")
    const std::string& bgPath = m_storyEngine->getCurrentBackground();
    if (!bgPath.empty()) {
        auto bgTex = ResourceManager::loadTexture(bgPath, resolveAsset(bgPath));
        if (bgTex) {
            m_spriteRenderer->drawSprite(*bgTex, glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f));
        } else {
            m_spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.11f, 0.13f, 0.20f, 1.0f));
        }
    } else {
        m_spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.11f, 0.13f, 0.20f, 1.0f));
    }

    // 2. Отрисовка персонажей с сортировкой по слоям (Z-index / Layer)
    // Персонажи на меньших слоях (layer 0) рисуются сзади, на больших (layer 1) — впереди!
    std::vector<CharacterSprite*> sortedCharacters;
    for (auto& [name, character] : m_characters) {
        if (character->isVisible()) {
            sortedCharacters.push_back(character.get());
        }
    }

    std::stable_sort(sortedCharacters.begin(), sortedCharacters.end(), [](const auto* a, const auto* b) {
        return a->getLayer() < b->getLayer();
    });

    for (auto* character : sortedCharacters) {
        character->render(*m_spriteRenderer, *m_textRenderer);
    }

    // 3. Отрисовка диалогового окна через выделенный компонент DialogueBox
    if (m_dialogueBox) {
        bool isWaitingClick = m_storyEngine->isWaitingForNextClick();
        m_dialogueBox->render(
            *m_spriteRenderer,
            *m_textRenderer,
            m_storyEngine->getCurrentSpeaker(),
            m_storyEngine->getVisibleText(),
            isWaitingClick
        );
    }

    // 6. Отрисовка интерактивных кнопок выбора (если наступила развилка)
    if (m_storyEngine->isWaitingForChoice()) {
        // Подложка динамического окна выбора
        m_spriteRenderer->drawRect(m_choiceWindowPos, m_choiceWindowSize, glm::vec4(0.05f, 0.06f, 0.10f, 0.94f));
        m_spriteRenderer->drawRect(m_choiceWindowPos, glm::vec2(m_choiceWindowSize.x, 3.0f), glm::vec4(0.95f, 0.55f, 0.25f, 1.0f));
        m_spriteRenderer->drawRect(m_choiceWindowPos + glm::vec2(0.0f, m_choiceWindowSize.y - 1.0f), glm::vec2(m_choiceWindowSize.x, 1.0f), glm::vec4(0.95f, 0.55f, 0.25f, 0.3f));

        // Разделитель под заголовком
        m_spriteRenderer->drawRect(glm::vec2(m_choiceWindowPos.x + 20.0f, m_choiceWindowPos.y + 52.0f), glm::vec2(m_choiceWindowSize.x - 40.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));

        // Заголовок окна выбора по центру верхней плашки
        std::string promptText = LOC("gameplay.choice_prompt");
        float titleW = m_textRenderer->measureText(promptText, 0.85f).x;
        float titleX = m_choiceWindowPos.x + (m_choiceWindowSize.x - titleW) * 0.5f;
        m_textRenderer->renderText(promptText, titleX, m_choiceWindowPos.y + 18.0f, 0.85f, glm::vec4(1.0f, 0.90f, 0.70f, 1.0f));

        for (auto& btn : m_choiceButtons) {
            btn.render(*m_spriteRenderer, *m_textRenderer);
        }
    } else {
        // Подсказка управления из локализации (выравнивание по правому нижнему краю диалогового окна)
        std::string controlsHint = LOC("gameplay.controls_hint");
        float hintW = m_textRenderer->measureText(controlsHint, 0.60f).x;
        m_textRenderer->renderText(controlsHint, 1220.0f - hintW, 655.0f, 0.60f, glm::vec4(0.5f, 0.55f, 0.65f, 0.7f));
    }
}

void Application::renderPauseMenu() {
    if (!m_spriteRenderer || !m_textRenderer) return;

    // 1. Полупрозрачное затемнение поверх всей игры (сквозь него видно размытую сцену)
    m_spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.40f));

    // 2. Карточка меню паузы (размер адаптирован под 6 кнопок)
    m_spriteRenderer->drawRect(glm::vec2(440.0f, 110.0f), glm::vec2(400.0f, 390.0f), glm::vec4(0.09f, 0.11f, 0.17f, 0.96f));
    
    // Рамка карточки
    float bw = 2.0f;
    glm::vec4 bc(0.35f, 0.55f, 0.95f, 0.9f);
    m_spriteRenderer->drawRect(glm::vec2(440.0f, 110.0f), glm::vec2(400.0f, bw), bc);
    m_spriteRenderer->drawRect(glm::vec2(440.0f, 500.0f - bw), glm::vec2(400.0f, bw), bc);
    m_spriteRenderer->drawRect(glm::vec2(440.0f, 110.0f), glm::vec2(bw, 390.0f), bc);
    m_spriteRenderer->drawRect(glm::vec2(840.0f - bw, 110.0f), glm::vec2(bw, 390.0f), bc);

    // Заголовок "ПАУЗА" из локализации
    std::string pauseTitle = LOC("pause.title");
    glm::vec2 titleSize = m_textRenderer->measureText(pauseTitle, 1.15f);
    float titleX = 440.0f + (400.0f - titleSize.x) * 0.5f;
    m_textRenderer->renderText(pauseTitle, titleX, 130.0f, 1.15f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Кнопки меню паузы
    for (auto& btn : m_pauseMenuButtons) {
        btn.render(*m_spriteRenderer, *m_textRenderer);
    }
}

void Application::renderBacklog() {
    if (!m_spriteRenderer || !m_textRenderer || !m_storyEngine) return;

    // 1. Полупрозрачное затемнение фона поверх размытой сцены
    m_spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.04f, 0.05f, 0.08f, 0.94f));

    // 2. Верхняя панель заголовка (Двухстрочный макет: заголовок сверху, подсказка снизу)
    m_textRenderer->renderText(LOC("backlog.title"), 80.0f, 14.0f, 0.95f, glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));
    m_textRenderer->renderText(LOC("backlog.hint"), 80.0f, 44.0f, 0.58f, glm::vec4(0.55f, 0.60f, 0.70f, 0.85f));

    if (m_backlogCloseButton) {
        m_backlogCloseButton->render(*m_spriteRenderer, *m_textRenderer);
    }

    // Декоративная разделительная полоса под шапкой
    m_spriteRenderer->drawRect(glm::vec2(80.0f, 70.0f), glm::vec2(1087.0f, 2.0f), glm::vec4(0.35f, 0.55f, 0.95f, 0.5f));

    // 3. Параметры области просмотра (Viewport)
    float boxX = 80.0f;
    float boxY = 78.0f;
    float boxW = 1060.0f;
    float boxH = 618.0f;
    float itemSpacing = 12.0f;
    float textMaxW = boxW - 40.0f; // 1020.0f
    float textScale = 0.72f;
    float speakerScale = 0.78f;

    const auto& history = m_storyEngine->getDialogueHistory();
    if (history.empty()) {
        std::string emptyMsg = LOC("backlog.empty");
        float emptyW = m_textRenderer->measureText(emptyMsg, 0.85f).x;
        m_textRenderer->renderText(emptyMsg, boxX + (boxW - emptyW) * 0.5f, boxY + 260.0f, 0.85f, glm::vec4(0.55f, 0.60f, 0.70f, 0.7f));
        return;
    }

    // Рассчитываем верстку карточек реплик
    struct BacklogItem {
        std::string wrappedText;
        float height = 0.0f;
    };

    std::vector<BacklogItem> items;
    items.reserve(history.size());
    float totalHeight = 0.0f;

    for (const auto& entry : history) {
        std::string wrapped = m_textRenderer->wrapText(entry.text, textMaxW, textScale);
        float textH = m_textRenderer->measureText(wrapped, textScale).y;
        float h = (entry.speaker.empty() ? 24.0f : 50.0f) + textH + 16.0f;
        items.push_back({std::move(wrapped), h});
        totalHeight += h + itemSpacing;
    }
    if (totalHeight > 0.0f) {
        totalHeight -= itemSpacing;
    }

    m_backlogMaxScrollY = std::max(0.0f, totalHeight - boxH);
    m_backlogScrollY = std::clamp(m_backlogScrollY, 0.0f, m_backlogMaxScrollY);

    // 4. Ограничение области отрисовки через OpenGL Scissor Box (с учетом масштабирования Retina/HiDPI)
    int fbWidth = 1280, fbHeight = 720;
    if (m_window) {
        m_window->getFramebufferSize(&fbWidth, &fbHeight);
    }
    float scaleX = static_cast<float>(fbWidth) / 1280.0f;
    float scaleY = static_cast<float>(fbHeight) / 720.0f;

    GLint scissorX = static_cast<GLint>(boxX * scaleX);
    GLint scissorY = static_cast<GLint>((720.0f - (boxY + boxH)) * scaleY);
    GLint scissorW = static_cast<GLint>(boxW * scaleX);
    GLint scissorH = static_cast<GLint>(boxH * scaleY);

    glEnable(GL_SCISSOR_TEST);
    glScissor(scissorX, scissorY, scissorW, scissorH);

    float currentY = boxY + 10.0f - m_backlogScrollY;
    for (size_t i = 0; i < history.size(); ++i) {
        const auto& entry = history[i];
        float itemH = items[i].height;

        // Отсечение невидимых элементов за границами экрана (Frustum culling)
        if (currentY + itemH >= boxY && currentY <= boxY + boxH) {
            // Фон карточки реплики
            m_spriteRenderer->drawRect(glm::vec2(boxX, currentY), glm::vec2(boxW, itemH), glm::vec4(0.07f, 0.09f, 0.14f, 0.82f));
            
            // Левая акцентная полоска (оранжевая для персонажей, серо-синяя для рассказчика)
            glm::vec4 accentColor = entry.speaker.empty() ? glm::vec4(0.40f, 0.50f, 0.65f, 0.7f) : glm::vec4(0.95f, 0.58f, 0.25f, 0.95f);
            m_spriteRenderer->drawRect(glm::vec2(boxX, currentY), glm::vec2(3.5f, itemH), accentColor);

            // Тонкая нижняя разделительная черта карточки
            m_spriteRenderer->drawRect(glm::vec2(boxX, currentY + itemH - 1.0f), glm::vec2(boxW, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.05f));

            float textStartY = currentY + 12.0f;
            if (!entry.speaker.empty()) {
                m_textRenderer->renderText(entry.speaker, boxX + 18.0f, currentY + 10.0f, speakerScale, glm::vec4(1.0f, 0.85f, 0.40f, 1.0f));
                textStartY = currentY + 40.0f;
            }

            m_textRenderer->renderText(items[i].wrappedText, boxX + 18.0f, textStartY, textScale, glm::vec4(0.92f, 0.94f, 0.98f, 1.0f));
        }

        currentY += itemH + itemSpacing;
    }

    glDisable(GL_SCISSOR_TEST);

    // 5. Отрисовка интерактивного скроллбара
    float trackX = 1155.0f;
    float trackY = 78.0f;
    float trackW = 12.0f;
    float trackH = 618.0f;

    // Дорожка скроллбара
    m_spriteRenderer->drawRect(glm::vec2(trackX, trackY), glm::vec2(trackW, trackH), glm::vec4(0.08f, 0.10f, 0.16f, 0.60f));

    // Ползунок (Thumb)
    if (m_backlogMaxScrollY > 0.0f) {
        float thumbH = std::max(40.0f, trackH * (trackH / (trackH + m_backlogMaxScrollY)));
        float scrollRatio = m_backlogScrollY / m_backlogMaxScrollY;
        float thumbY = trackY + scrollRatio * (trackH - thumbH);
        glm::vec4 thumbColor = m_isDraggingScrollbar ? glm::vec4(0.60f, 0.80f, 1.0f, 0.95f) : glm::vec4(0.35f, 0.55f, 0.95f, 0.80f);
        m_spriteRenderer->drawRect(glm::vec2(trackX, thumbY), glm::vec2(trackW, thumbH), thumbColor);
    } else {
        m_spriteRenderer->drawRect(glm::vec2(trackX, trackY), glm::vec2(trackW, trackH), glm::vec4(0.20f, 0.25f, 0.35f, 0.30f));
    }
}

void Application::openSettingsMenu(GameState returnState) {
    m_settingsReturnState = returnState;
    m_gameState = GameState::SettingsMenu;
    m_activeDraggingSlider = -1;
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

void Application::openSaveMenu() {
    m_saveLoadReturnState = GameState::PauseMenu;
    m_gameState = GameState::SaveMenu;
    refreshSaveSlotsMetadata();
}

void Application::openLoadMenu(GameState returnState) {
    m_saveLoadReturnState = returnState;
    m_gameState = GameState::LoadMenu;
    refreshSaveSlotsMetadata();
}

void Application::refreshSaveSlotsMetadata() {
    m_cachedSlots = SaveManager::getSlotsMetadata(SaveManager::DEFAULT_MAX_SLOTS);
    m_cachedQuickSave = SaveManager::getQuickSaveMetadata();
    for (int slot = 1; slot <= SaveManager::DEFAULT_MAX_SLOTS; ++slot) {
        ResourceManager::removeTexture("slot_" + std::to_string(slot));
    }
    ResourceManager::removeTexture("quicksave");
}

void Application::showToast(const std::string& message) {
    m_toastMessage = message;
    m_toastTimer = 2.5f;
}

void Application::captureScreenThumbnail(std::vector<uint8_t>& outPixels, int& outW, int& outH) {
    if (m_renderPipeline) {
        int fboW = m_renderPipeline->getSceneFBO().getWidth();
        int fboH = m_renderPipeline->getSceneFBO().getHeight();
        outW = fboW;
        outH = fboH;
        outPixels.resize(static_cast<size_t>(fboW * fboH * 3));

        // Отрисовываем чистый кадр сцены (фон, персонажи, текст диалога) во внеэкранный буфер
        m_renderPipeline->beginScene();
        renderGameplay();

        // ВАЖНО: читаем пиксели прямо из привязанного m_sceneFBO ДО вызова endScene(),
        // чтобы прочитать чистый кадр игрового процесса без наложенного оверлея меню сохранения!
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, fboW, fboH, GL_RGB, GL_UNSIGNED_BYTE, outPixels.data());

        m_renderPipeline->endScene();
    } else {
        int fbW = 1280, fbH = 720;
        if (m_window) {
            m_window->getFramebufferSize(&fbW, &fbH);
        }
        outW = fbW;
        outH = fbH;
        outPixels.resize(static_cast<size_t>(fbW * fbH * 3));

        glViewport(0, 0, fbW, fbH);
        glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        renderGameplay();

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, fbW, fbH, GL_RGB, GL_UNSIGNED_BYTE, outPixels.data());
    }
}

void Application::performSave(int slotIndex) {
    if (!m_storyEngine) return;
    GameSaveState state;
    m_storyEngine->captureSaveState(state);

    if (m_audioEngine) {
        state.bgmPath = m_audioEngine->getCurrentMusicPath();
        state.bgmVolume = m_audioEngine->getBGMVolume();
        state.bgmLoop = true;
        state.ambientPath = m_audioEngine->getCurrentAmbientPath();
        state.ambientVolume = m_audioEngine->getAmbientVolume();
        state.ambientLoop = true;
    }

    std::vector<uint8_t> pixels;
    int scrW = 0, scrH = 0;
    captureScreenThumbnail(pixels, scrW, scrH);

    if (SaveManager::saveSlot(slotIndex, state, pixels.data(), scrW, scrH)) {
        refreshSaveSlotsMetadata();
        if (slotIndex == SaveManager::QUICKSAVE_SLOT_INDEX) {
            showToast(LOC("saveload.quicksaved_toast"));
        } else {
            std::string toastFmt = LOC("saveload.saved_toast");
            size_t pos = toastFmt.find("{}");
            if (pos != std::string::npos) {
                toastFmt.replace(pos, 2, std::to_string(slotIndex));
            }
            showToast(toastFmt);
        }
    }
}

void Application::performLoad(int slotIndex) {
    if (!m_storyEngine) return;
    GameSaveState state;
    if (!SaveManager::loadSlot(slotIndex, state)) {
        std::cerr << "[Application] Failed to load slot " << slotIndex << std::endl;
        return;
    }

    m_characters.clear();
    m_choiceButtons.clear();

    if (m_storyEngine->restoreSaveState(state)) {
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

        if (m_storyEngine->isWaitingForChoice()) {
            updateChoiceButtons();
        }

        m_gameState = GameState::Gameplay;
        showToast(LOC("saveload.loaded_toast"));
    }
}

void Application::renderSettingsMenu() {
    if (!m_spriteRenderer || !m_textRenderer) return;

    // 1. Затемнение фона поверх размытия сцены
    m_spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.04f, 0.05f, 0.08f, 0.94f));

    // 2. Верхняя панель заголовка
    m_textRenderer->renderText(LOC("settings.title"), 80.0f, 14.0f, 0.95f, glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));
    m_textRenderer->renderText(LOC("settings.hint"), 80.0f, 44.0f, 0.58f, glm::vec4(0.55f, 0.60f, 0.70f, 0.85f));

    if (m_settingsResetButton) {
        m_settingsResetButton->render(*m_spriteRenderer, *m_textRenderer);
    }
    if (m_settingsBackButton) {
        m_settingsBackButton->render(*m_spriteRenderer, *m_textRenderer);
    }

    // Декоративная разделительная полоса
    m_spriteRenderer->drawRect(glm::vec2(80.0f, 70.0f), glm::vec2(1120.0f, 2.0f), glm::vec4(0.35f, 0.55f, 0.95f, 0.5f));

    glm::vec2 mousePos = Input::getMousePosition();

    // -------------------------------------------------------------
    // Левая колонка: Аудио
    // -------------------------------------------------------------
    m_textRenderer->renderText(LOC("settings.section_audio"), 115.0f, 95.0f, 0.82f, glm::vec4(0.95f, 0.80f, 0.40f, 1.0f));
    m_spriteRenderer->drawRect(glm::vec2(115.0f, 125.0f), glm::vec2(490.0f, 1.0f), glm::vec4(0.35f, 0.45f, 0.60f, 0.35f));

    struct SliderDef {
        std::string labelKey;
        std::string configKey;
        float defaultVal;
    };
    SliderDef audioSliders[5] = {
        {"settings.master_volume",  "audio.master_volume",  1.0f},
        {"settings.bgm_volume",     "audio.bgm_volume",     0.7f},
        {"settings.sfx_volume",     "audio.sfx_volume",     0.8f},
        {"settings.voice_volume",   "audio.voice_volume",   0.9f},
        {"settings.ambient_volume", "audio.ambient_volume", 0.6f}
    };

    float sliderStartY = 145.0f;
    float sliderStepY = 85.0f;
    float audioTrackX = 115.0f;
    float audioTrackW = 490.0f;
    float trackH = 8.0f;

    for (int i = 0; i < 5; ++i) {
        float y = sliderStartY + static_cast<float>(i) * sliderStepY;
        float val = Config::getFloat(audioSliders[i].configKey, audioSliders[i].defaultVal);
        val = std::clamp(val, 0.0f, 1.0f);

        m_textRenderer->renderText(LOC(audioSliders[i].labelKey), audioTrackX, y, 0.68f, glm::vec4(0.90f, 0.92f, 0.96f, 1.0f));

        int percent = static_cast<int>(std::round(val * 100.0f));
        std::string valStr = std::to_string(percent) + "%";
        float valW = m_textRenderer->measureText(valStr, 0.65f).x;
        m_textRenderer->renderText(valStr, audioTrackX + audioTrackW - valW, y, 0.65f, glm::vec4(0.45f, 0.75f, 1.0f, 1.0f));

        float trackY = y + 28.0f;
        m_spriteRenderer->drawRect(glm::vec2(audioTrackX, trackY), glm::vec2(audioTrackW, trackH), glm::vec4(0.12f, 0.15f, 0.22f, 1.0f));
        m_spriteRenderer->drawRect(glm::vec2(audioTrackX, trackY), glm::vec2(audioTrackW, 1.0f), glm::vec4(0.25f, 0.35f, 0.50f, 0.6f));
        m_spriteRenderer->drawRect(glm::vec2(audioTrackX, trackY + trackH - 1.0f), glm::vec2(audioTrackW, 1.0f), glm::vec4(0.25f, 0.35f, 0.50f, 0.6f));

        float fillW = audioTrackW * val;
        if (fillW > 0.0f) {
            m_spriteRenderer->drawRect(glm::vec2(audioTrackX, trackY), glm::vec2(fillW, trackH), glm::vec4(0.25f, 0.55f, 0.95f, 1.0f));
        }

        float handleW = 16.0f;
        float handleH = 22.0f;
        float handleX = audioTrackX + fillW - handleW * 0.5f;
        float handleY = trackY + trackH * 0.5f - handleH * 0.5f;
        bool isHovered = (mousePos.x >= handleX - 4.0f && mousePos.x <= handleX + handleW + 4.0f &&
                          mousePos.y >= handleY - 4.0f && mousePos.y <= handleY + handleH + 4.0f) ||
                         (m_activeDraggingSlider == i);

        glm::vec4 handleColor = isHovered ? glm::vec4(1.0f, 0.85f, 0.40f, 1.0f) : glm::vec4(0.90f, 0.93f, 0.98f, 1.0f);
        m_spriteRenderer->drawRect(glm::vec2(handleX, handleY), glm::vec2(handleW, handleH), handleColor);
        m_spriteRenderer->drawRect(glm::vec2(handleX + 2.0f, handleY + 2.0f), glm::vec2(handleW - 4.0f, handleH - 4.0f), glm::vec4(0.10f, 0.14f, 0.22f, 1.0f));
    }

    // -------------------------------------------------------------
    // Правая колонка: Геймплей и Экран
    // -------------------------------------------------------------
    m_textRenderer->renderText(LOC("settings.section_gameplay"), 675.0f, 95.0f, 0.82f, glm::vec4(0.95f, 0.80f, 0.40f, 1.0f));
    m_spriteRenderer->drawRect(glm::vec2(675.0f, 125.0f), glm::vec2(480.0f, 1.0f), glm::vec4(0.35f, 0.45f, 0.60f, 0.35f));

    // Слайдер скорости текста (0.005s .. 0.060s)
    float textSpeedVal = Config::getFloat("story.text_speed", 0.030f);
    float textT = (0.060f - textSpeedVal) / (0.060f - 0.005f);
    textT = std::clamp(textT, 0.0f, 1.0f);

    m_textRenderer->renderText(LOC("settings.text_speed"), 675.0f, 145.0f, 0.68f, glm::vec4(0.90f, 0.92f, 0.96f, 1.0f));
    int speedPercent = static_cast<int>(std::round(textT * 100.0f));
    std::string speedStr = std::to_string(speedPercent) + "%";
    float speedW = m_textRenderer->measureText(speedStr, 0.65f).x;
    m_textRenderer->renderText(speedStr, 675.0f + 480.0f - speedW, 145.0f, 0.65f, glm::vec4(0.45f, 0.75f, 1.0f, 1.0f));

    float textTrackY = 145.0f + 28.0f;
    m_spriteRenderer->drawRect(glm::vec2(675.0f, textTrackY), glm::vec2(480.0f, trackH), glm::vec4(0.12f, 0.15f, 0.22f, 1.0f));
    m_spriteRenderer->drawRect(glm::vec2(675.0f, textTrackY), glm::vec2(480.0f, 1.0f), glm::vec4(0.25f, 0.35f, 0.50f, 0.6f));
    m_spriteRenderer->drawRect(glm::vec2(675.0f, textTrackY + trackH - 1.0f), glm::vec2(480.0f, 1.0f), glm::vec4(0.25f, 0.35f, 0.50f, 0.6f));

    float textFillW = 480.0f * textT;
    if (textFillW > 0.0f) {
        m_spriteRenderer->drawRect(glm::vec2(675.0f, textTrackY), glm::vec2(textFillW, trackH), glm::vec4(0.25f, 0.55f, 0.95f, 1.0f));
    }
    float textHandleW = 16.0f;
    float textHandleH = 22.0f;
    float textHandleX = 675.0f + textFillW - textHandleW * 0.5f;
    float textHandleY = textTrackY + trackH * 0.5f - textHandleH * 0.5f;
    bool isTextHandleHovered = (mousePos.x >= textHandleX - 4.0f && mousePos.x <= textHandleX + textHandleW + 4.0f &&
                                mousePos.y >= textHandleY - 4.0f && mousePos.y <= textHandleY + textHandleH + 4.0f) ||
                               (m_activeDraggingSlider == 5);
    glm::vec4 textHandleColor = isTextHandleHovered ? glm::vec4(1.0f, 0.85f, 0.40f, 1.0f) : glm::vec4(0.90f, 0.93f, 0.98f, 1.0f);
    m_spriteRenderer->drawRect(glm::vec2(textHandleX, textHandleY), glm::vec2(textHandleW, textHandleH), textHandleColor);
    m_spriteRenderer->drawRect(glm::vec2(textHandleX + 2.0f, textHandleY + 2.0f), glm::vec2(textHandleW - 4.0f, textHandleH - 4.0f), glm::vec4(0.10f, 0.14f, 0.22f, 1.0f));

    // Выбор языка
    m_textRenderer->renderText(LOC("settings.language"), 675.0f, 235.0f, 0.68f, glm::vec4(0.90f, 0.92f, 0.96f, 1.0f));
    auto langs = Localization::getAvailableLanguages();
    if (langs.empty()) langs = {"ru", "en", "uk"};
    std::string curLang = Localization::getCurrentLanguage();
    float langBtnX = 675.0f;
    float langBtnY = 235.0f + 26.0f;
    float langBtnW = 145.0f;
    float langBtnH = 36.0f;
    float langBtnSpacing = 16.0f;

    for (size_t i = 0; i < langs.size(); ++i) {
        float bx = langBtnX + static_cast<float>(i) * (langBtnW + langBtnSpacing);
        bool isCur = (langs[i] == curLang);
        bool isHover = (mousePos.x >= bx && mousePos.x <= bx + langBtnW &&
                        mousePos.y >= langBtnY && mousePos.y <= langBtnY + langBtnH);

        glm::vec4 btnBg = isCur ? glm::vec4(0.18f, 0.35f, 0.65f, 0.95f)
                                : (isHover ? glm::vec4(0.15f, 0.19f, 0.28f, 0.90f) : glm::vec4(0.08f, 0.11f, 0.17f, 0.85f));
        glm::vec4 btnBorder = isCur ? glm::vec4(0.95f, 0.75f, 0.30f, 1.0f)
                                    : (isHover ? glm::vec4(0.45f, 0.65f, 0.95f, 0.8f) : glm::vec4(0.25f, 0.32f, 0.45f, 0.5f));

        m_spriteRenderer->drawRect(glm::vec2(bx, langBtnY), glm::vec2(langBtnW, langBtnH), btnBg);
        m_spriteRenderer->drawRect(glm::vec2(bx, langBtnY), glm::vec2(langBtnW, 1.5f), btnBorder);
        m_spriteRenderer->drawRect(glm::vec2(bx, langBtnY + langBtnH - 1.5f), glm::vec2(langBtnW, 1.5f), btnBorder);
        m_spriteRenderer->drawRect(glm::vec2(bx, langBtnY), glm::vec2(1.5f, langBtnH), btnBorder);
        m_spriteRenderer->drawRect(glm::vec2(bx + langBtnW - 1.5f, langBtnY), glm::vec2(1.5f, langBtnH), btnBorder);

        std::string label = (langs[i] == "ru" ? "Русский" : (langs[i] == "en" ? "English" : "Українська"));
        float txtW = m_textRenderer->measureText(label, 0.62f).x;
        glm::vec4 txtCol = isCur ? glm::vec4(1.0f, 0.95f, 0.80f, 1.0f) : glm::vec4(0.80f, 0.85f, 0.92f, 0.9f);
        m_textRenderer->renderText(label, bx + (langBtnW - txtW) * 0.5f, langBtnY + 8.0f, 0.62f, txtCol);
    }

    // Тумблеры (VSync, Fullscreen, Auto-dim)
    struct ToggleDef {
        std::string labelKey;
        bool state;
        float y;
    };
    ToggleDef toggles[3] = {
        {"settings.vsync",      Config::getBool("window.vsync", true),                                          340.0f},
        {"settings.fullscreen", m_window ? m_window->isFullscreen() : Config::getBool("window.fullscreen", false), 420.0f},
        {"settings.auto_dim",   Config::getBool("story.auto_dim_inactive", false),                              500.0f}
    };

    for (const auto& tog : toggles) {
        m_textRenderer->renderText(LOC(tog.labelKey), 675.0f, tog.y + 6.0f, 0.68f, glm::vec4(0.90f, 0.92f, 0.96f, 1.0f));

        float togX = 1045.0f;
        float togW = 100.0f;
        float togH = 34.0f;
        bool isHover = (mousePos.x >= togX && mousePos.x <= togX + togW &&
                        mousePos.y >= tog.y && mousePos.y <= tog.y + togH);

        glm::vec4 bgCol = tog.state ? (isHover ? glm::vec4(0.20f, 0.52f, 0.30f, 0.95f) : glm::vec4(0.15f, 0.42f, 0.24f, 0.90f))
                                    : (isHover ? glm::vec4(0.22f, 0.15f, 0.17f, 0.95f) : glm::vec4(0.16f, 0.11f, 0.13f, 0.90f));
        glm::vec4 brdCol = tog.state ? glm::vec4(0.35f, 0.88f, 0.50f, 1.0f) : glm::vec4(0.65f, 0.30f, 0.35f, 0.8f);

        m_spriteRenderer->drawRect(glm::vec2(togX, tog.y), glm::vec2(togW, togH), bgCol);
        m_spriteRenderer->drawRect(glm::vec2(togX, tog.y), glm::vec2(togW, 1.5f), brdCol);
        m_spriteRenderer->drawRect(glm::vec2(togX, tog.y + togH - 1.5f), glm::vec2(togW, 1.5f), brdCol);
        m_spriteRenderer->drawRect(glm::vec2(togX, tog.y), glm::vec2(1.5f, togH), brdCol);
        m_spriteRenderer->drawRect(glm::vec2(togX + togW - 1.5f, tog.y), glm::vec2(1.5f, togH), brdCol);

        std::string txt = tog.state ? LOC("settings.on") : LOC("settings.off");
        float tw = m_textRenderer->measureText(txt, 0.62f).x;
        glm::vec4 textCol = tog.state ? glm::vec4(0.85f, 1.0f, 0.85f, 1.0f) : glm::vec4(0.95f, 0.70f, 0.75f, 0.9f);
        m_textRenderer->renderText(txt, togX + (togW - tw) * 0.5f, tog.y + 7.0f, 0.62f, textCol);
    }
}

void Application::renderSaveLoadMenu(bool isSaving) {
    if (!m_spriteRenderer || !m_textRenderer) return;

    // 1. Затемнение фона поверх размытия сцены
    m_spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.04f, 0.05f, 0.08f, 0.94f));

    // 2. Верхняя панель заголовка
    std::string titleStr = isSaving ? LOC("saveload.save_title") : LOC("saveload.load_title");
    std::string hintStr = isSaving ? LOC("saveload.hint_save") : LOC("saveload.hint_load");
    m_textRenderer->renderText(titleStr, 80.0f, 14.0f, 0.95f, glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));
    m_textRenderer->renderText(hintStr, 80.0f, 44.0f, 0.58f, glm::vec4(0.55f, 0.60f, 0.70f, 0.85f));

    if (m_saveLoadBackButton) {
        m_saveLoadBackButton->render(*m_spriteRenderer, *m_textRenderer);
    }

    // Декоративная разделительная полоса
    m_spriteRenderer->drawRect(glm::vec2(80.0f, 70.0f), glm::vec2(1087.0f, 2.0f), glm::vec4(0.35f, 0.55f, 0.95f, 0.5f));

    // 3. Отрисовка 6 слотов в сетке 3x2
    glm::vec2 mousePos = Input::getMousePosition();
    float startX = 80.0f;
    float startY = 88.0f;
    float cardW = 350.0f;
    float cardH = 270.0f;
    float spacingX = 35.0f;
    float spacingY = 24.0f;

    for (int slot = 1; slot <= 6; ++slot) {
        int col = (slot - 1) % 3;
        int row = (slot - 1) / 3;
        float x = startX + static_cast<float>(col) * (cardW + spacingX);
        float y = startY + static_cast<float>(row) * (cardH + spacingY);

        bool isHovered = (mousePos.x >= x && mousePos.x <= x + cardW &&
                          mousePos.y >= y && mousePos.y <= y + cardH);

        size_t metaIdx = static_cast<size_t>(slot - 1);
        const SaveSlotMetadata* meta = (metaIdx < m_cachedSlots.size()) ? &m_cachedSlots[metaIdx] : nullptr;
        bool exists = meta && meta->exists;

        // Фон карточки
        glm::vec4 bgColor = isHovered ? glm::vec4(0.10f, 0.13f, 0.20f, 0.96f) : glm::vec4(0.07f, 0.09f, 0.14f, 0.88f);
        m_spriteRenderer->drawRect(glm::vec2(x, y), glm::vec2(cardW, cardH), bgColor);

        // Рамка карточки
        glm::vec4 borderColor = isHovered ? glm::vec4(0.95f, 0.65f, 0.25f, 1.0f) : glm::vec4(0.25f, 0.35f, 0.55f, 0.45f);
        float bw = isHovered ? 2.5f : 1.5f;
        m_spriteRenderer->drawRect(glm::vec2(x, y), glm::vec2(cardW, bw), borderColor);
        m_spriteRenderer->drawRect(glm::vec2(x, y + cardH - bw), glm::vec2(cardW, bw), borderColor);
        m_spriteRenderer->drawRect(glm::vec2(x, y), glm::vec2(bw, cardH), borderColor);
        m_spriteRenderer->drawRect(glm::vec2(x + cardW - bw, y), glm::vec2(bw, cardH), borderColor);

        // Заголовок слота ("Слот 1")
        std::string slotTitle = LOC("saveload.slot_prefix") + " " + std::to_string(slot);
        m_textRenderer->renderText(slotTitle, x + 16.0f, y + 12.0f, 0.75f, glm::vec4(1.0f, 0.85f, 0.40f, 1.0f));

        if (exists) {
            // Дата и время сохранения
            m_textRenderer->renderText(meta->timestamp, x + 120.0f, y + 16.0f, 0.58f, glm::vec4(0.60f, 0.65f, 0.75f, 0.9f));

            // Кнопка удаления (✕)
            bool delHover = (mousePos.x >= x + cardW - 36.0f && mousePos.x <= x + cardW - 12.0f &&
                             mousePos.y >= y + 8.0f && mousePos.y <= y + 32.0f);
            glm::vec4 delColor = delHover ? glm::vec4(0.95f, 0.35f, 0.35f, 1.0f) : glm::vec4(0.55f, 0.60f, 0.70f, 0.65f);
            m_textRenderer->renderText("✕", x + cardW - 30.0f, y + 12.0f, 0.75f, delColor);

            // Миниатюра-скриншот
            float thumbX = x + 16.0f;
            float thumbY = y + 42.0f;
            float thumbW = cardW - 32.0f; // 318.0f
            float thumbH = 135.0f;

            if (!meta->screenshotPath.empty()) {
                auto tex = ResourceManager::loadTexture("slot_" + std::to_string(slot), meta->screenshotPath);
                if (tex) {
                    m_spriteRenderer->drawSprite(*tex, glm::vec2(thumbX, thumbY), glm::vec2(thumbW, thumbH));
                } else {
                    m_spriteRenderer->drawRect(glm::vec2(thumbX, thumbY), glm::vec2(thumbW, thumbH), glm::vec4(0.04f, 0.05f, 0.08f, 0.9f));
                }
            } else {
                m_spriteRenderer->drawRect(glm::vec2(thumbX, thumbY), glm::vec2(thumbW, thumbH), glm::vec4(0.04f, 0.05f, 0.08f, 0.9f));
            }

            // Тонкая рамка вокруг миниатюры
            m_spriteRenderer->drawRect(glm::vec2(thumbX, thumbY), glm::vec2(thumbW, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));
            m_spriteRenderer->drawRect(glm::vec2(thumbX, thumbY + thumbH - 1.0f), glm::vec2(thumbW, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));
            m_spriteRenderer->drawRect(glm::vec2(thumbX, thumbY), glm::vec2(1.0f, thumbH), glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));
            m_spriteRenderer->drawRect(glm::vec2(thumbX + thumbW - 1.0f, thumbY), glm::vec2(1.0f, thumbH), glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));

            // Превью реплики под скриншотом
            float textY = y + 186.0f;
            if (!meta->previewSpeaker.empty()) {
                m_textRenderer->renderText(meta->previewSpeaker, x + 16.0f, textY, 0.65f, glm::vec4(0.95f, 0.80f, 0.40f, 1.0f));
                textY += 22.0f;
            }

            std::string previewText = meta->previewText;
            std::string wrapped = m_textRenderer->wrapText(previewText, thumbW, 0.56f);
            m_textRenderer->renderText(wrapped, x + 16.0f, textY, 0.56f, glm::vec4(0.85f, 0.88f, 0.92f, 0.9f));
        } else {
            // Пустой слот
            float thumbX = x + 16.0f;
            float thumbY = y + 42.0f;
            float thumbW = cardW - 32.0f;
            float thumbH = 135.0f;

            m_spriteRenderer->drawRect(glm::vec2(thumbX, thumbY), glm::vec2(thumbW, thumbH), glm::vec4(0.04f, 0.05f, 0.08f, 0.5f));
            std::string emptyLabel = LOC("saveload.empty");
            float emptyW = m_textRenderer->measureText(emptyLabel, 0.70f).x;
            m_textRenderer->renderText(emptyLabel, thumbX + (thumbW - emptyW) * 0.5f, thumbY + 54.0f, 0.70f, glm::vec4(0.40f, 0.45f, 0.55f, 0.7f));
        }
    }
}

void Application::shutdown() {
    m_mainMenuButtons.clear();
    m_pauseMenuButtons.clear();
    m_backlogCloseButton.reset();
    m_saveLoadBackButton.reset();
    m_cachedSlots.clear();
    m_choiceButtons.clear();
    m_characters.clear();

    Localization::clear();
    m_storyEngine.reset();
    m_textRenderer.reset();
    m_spriteRenderer.reset();
    ResourceManager::clear();

    m_pythonEngine.reset();
    m_scene.reset();
    m_renderPipeline.reset();
    m_window.reset();
}
