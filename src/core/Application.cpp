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

    int winWidth = Config::getInt("window.width", 1280);
    int winHeight = Config::getInt("window.height", 720);
    std::string winTitle = Config::getString("window.title", "DariVN - Visual Novel Engine");

    m_window = std::make_unique<Window>(winWidth, winHeight, winTitle);
    if (!m_window->is_valid()) {
        std::cerr << "[Application] Window initialization error" << std::endl;
        return false;
    }
    m_window->setVSync(Config::getBool("window.vsync", true));

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
        glm::vec2(100.0f, 290.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.new_game"),
        [this]() {
            std::string startScript = Config::getString("start_script", "");
            if (startScript.empty()) {
                startScript = Config::getString("story.start_script", "assets/scripts/demo.vn");
            }
            m_characters.clear();
            m_choiceButtons.clear();
            m_storyEngine->loadScript(resolveAsset(startScript));
            m_storyEngine->executeNextCommand();
            m_gameState = GameState::Gameplay;
        }
    );

    m_mainMenuButtons.emplace_back(
        glm::vec2(100.0f, 355.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.load"),
        []() {
            std::cout << "[UI] Меню сохранений (в разработке)" << std::endl;
        }
    );

    m_mainMenuButtons.emplace_back(
        glm::vec2(100.0f, 420.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.settings"),
        []() {
            std::cout << "[UI] Меню настроек (в разработке)" << std::endl;
        }
    );

    // Кнопка переключения языка прямо в главном меню (цикл по всем доступным языкам из конфига)
    m_mainMenuButtons.emplace_back(
        glm::vec2(100.0f, 485.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.lang_switch"),
        [this]() {
            auto langs = Localization::getAvailableLanguages();
            if (langs.size() > 1) {
                std::string current = Localization::getCurrentLanguage();
                auto it = std::find(langs.begin(), langs.end(), current);
                size_t nextIdx = (it != langs.end()) ? ((it - langs.begin() + 1) % langs.size()) : 0;
                Localization::setLanguage(langs[nextIdx]);
                initUI(); // Мгновенно пересоздаем кнопки на выбранном языке
            }
        }
    );

    m_mainMenuButtons.emplace_back(
        glm::vec2(100.0f, 550.0f), glm::vec2(280.0f, 50.0f),
        LOC("menu.exit"),
        [this]() {
            m_isRunning = false;
        }
    );

    // -------------------------------------------------------------
    // Кнопки Меню Паузы
    // -------------------------------------------------------------
    m_pauseMenuButtons.clear();

    m_pauseMenuButtons.emplace_back(
        glm::vec2(490.0f, 280.0f), glm::vec2(300.0f, 52.0f),
        LOC("pause.resume"),
        [this]() {
            m_gameState = GameState::Gameplay;
        }
    );

    m_pauseMenuButtons.emplace_back(
        glm::vec2(490.0f, 350.0f), glm::vec2(300.0f, 52.0f),
        LOC("pause.main_menu"),
        [this]() {
            m_gameState = GameState::MainMenu;
            if (m_audioEngine) {
                m_audioEngine->stopAmbient(1.0f);
                std::string menuMusic = Config::getString("menu.music", "");
                if (!menuMusic.empty()) {
                    m_audioEngine->playMusic(resolveAsset(menuMusic), true, m_audioEngine->getDefaultBGMFade());
                }
            }
        }
    );

    m_pauseMenuButtons.emplace_back(
        glm::vec2(490.0f, 420.0f), glm::vec2(300.0f, 52.0f),
        LOC("pause.exit"),
        [this]() {
            m_isRunning = false;
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
            // Открытие паузы по Escape
            if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE)) {
                m_gameState = GameState::PauseMenu;
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
                break;
            }

            for (auto& btn : m_pauseMenuButtons) {
                btn.update(mousePos, mouseClicked);
            }
            break;
        }
    }
}

void Application::update(float deltaTime) {
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
        // Подсказка управления из локализации
        m_textRenderer->renderText(LOC("gameplay.controls_hint"), 720.0f, 650.0f, 0.65f, glm::vec4(0.5f, 0.55f, 0.65f, 0.7f));
    }
}

void Application::renderPauseMenu() {
    if (!m_spriteRenderer || !m_textRenderer) return;

    // 1. Полупрозрачное затемнение поверх всей игры (сквозь него видно размытую сцену)
    m_spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.40f));

    // 2. Карточка меню паузы
    m_spriteRenderer->drawRect(glm::vec2(440.0f, 170.0f), glm::vec2(400.0f, 350.0f), glm::vec4(0.09f, 0.11f, 0.17f, 0.96f));
    
    // Рамка карточки
    float bw = 2.0f;
    glm::vec4 bc(0.35f, 0.55f, 0.95f, 0.9f);
    m_spriteRenderer->drawRect(glm::vec2(440.0f, 170.0f), glm::vec2(400.0f, bw), bc);
    m_spriteRenderer->drawRect(glm::vec2(440.0f, 520.0f - bw), glm::vec2(400.0f, bw), bc);
    m_spriteRenderer->drawRect(glm::vec2(440.0f, 170.0f), glm::vec2(bw, 350.0f), bc);
    m_spriteRenderer->drawRect(glm::vec2(840.0f - bw, 170.0f), glm::vec2(bw, 350.0f), bc);

    // Заголовок "ПАУЗА" из локализации
    std::string pauseTitle = LOC("pause.title");
    glm::vec2 titleSize = m_textRenderer->measureText(pauseTitle, 1.2f);
    float titleX = 440.0f + (400.0f - titleSize.x) * 0.5f;
    m_textRenderer->renderText(pauseTitle, titleX, 210.0f, 1.2f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Кнопки меню паузы
    for (auto& btn : m_pauseMenuButtons) {
        btn.render(*m_spriteRenderer, *m_textRenderer);
    }
}

void Application::shutdown() {
    m_mainMenuButtons.clear();
    m_pauseMenuButtons.clear();
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
