#include "SettingsState.hpp"
#include "StateManager.hpp"

#include "core/Config.hpp"
#include "core/Input.hpp"
#include "core/Window.hpp"
#include "resource/Localization.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"
#include "audio/AudioEngine.hpp"
#include "script/StoryEngine.hpp"

#include <GLFW/glfw3.h>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

SettingsState::SettingsState(SharedContext& context)
    : IGameState(context) {
}

void SettingsState::onEnter() {
    initButtons();
    m_activeDraggingSlider = -1;
}

void SettingsState::initButtons() {
    m_backButton = std::make_unique<Button>(
        glm::vec2(1070.0f, 14.0f), glm::vec2(130.0f, 44.0f),
        LOC("settings.back"),
        [this]() {
            Config::saveUserSettings();
            if (m_context.showToast) {
                m_context.showToast(LOC("settings.saved_toast"));
            }
            m_context.stateManager->popState();
        }
    );

    m_resetButton = std::make_unique<Button>(
        glm::vec2(925.0f, 14.0f), glm::vec2(130.0f, 44.0f),
        LOC("settings.defaults"),
        [this]() {
            Config::resetUserSettings();
            if (m_context.applySettings) {
                m_context.applySettings();
            }
            initButtons();
            if (m_context.showToast) {
                m_context.showToast(LOC("settings.reset_toast"));
            }
        }
    );
}

void SettingsState::handleInput() {
    // 1. Закрытие по Escape или ПКМ -> сохранение и возврат
    if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE) || Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        Config::saveUserSettings();
        if (m_context.showToast) {
            m_context.showToast(LOC("settings.saved_toast"));
        }
        m_context.stateManager->popState();
        return;
    }

    glm::vec2 mousePos = Input::getMousePosition();
    bool mouseClicked = Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
    bool isLeftPressed = Input::isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);

    // 2. Кнопка "Назад"
    if (m_backButton) {
        m_backButton->update(mousePos, mouseClicked);
    }

    // 3. Кнопка "Сброс"
    if (m_resetButton) {
        m_resetButton->update(mousePos, mouseClicked);
    }

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
            if (m_context.applyAudioSettings) {
                m_context.applyAudioSettings();
            }
        } else if (m_activeDraggingSlider == 5) {
            float textTrackX = 675.0f;
            float textTrackW = 480.0f;
            float t = (mousePos.x - textTrackX) / textTrackW;
            t = std::clamp(t, 0.0f, 1.0f);
            float speed = 0.060f - t * (0.060f - 0.005f);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(3) << speed;
            Config::setUserSetting("story.text_speed", ss.str());
            if (m_context.storyEngine) {
                m_context.storyEngine->setTextSpeed(speed);
            }
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
                initButtons();
                break;
            }
        }

        // Тумблер VSync
        if (mousePos.x >= 1045.0f && mousePos.x <= 1145.0f &&
            mousePos.y >= 340.0f && mousePos.y <= 374.0f) {
            bool cur = Config::getBool("window.vsync", true);
            bool next = !cur;
            Config::setUserSetting("window.vsync", next ? "true" : "false");
            if (m_context.window) m_context.window->setVSync(next);
            Config::saveUserSettings();
        }

        // Тумблер Полноэкранный режим
        if (mousePos.x >= 1045.0f && mousePos.x <= 1145.0f &&
            mousePos.y >= 420.0f && mousePos.y <= 454.0f) {
            bool cur = m_context.window ? m_context.window->isFullscreen() : Config::getBool("window.fullscreen", false);
            bool next = !cur;
            Config::setUserSetting("window.fullscreen", next ? "true" : "false");
            if (m_context.window) m_context.window->setFullscreen(next);
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
}

void SettingsState::update(float dt) {
    (void)dt;
}

void SettingsState::render() {
    if (!m_context.spriteRenderer || !m_context.textRenderer) return;

    // 1. Затемнение фона поверх размытия сцены
    m_context.spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.04f, 0.05f, 0.08f, 0.94f));

    // 2. Верхняя панель заголовка
    m_context.textRenderer->renderText(LOC("settings.title"), 80.0f, 14.0f, 0.95f, glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));
    m_context.textRenderer->renderText(LOC("settings.hint"), 80.0f, 44.0f, 0.58f, glm::vec4(0.55f, 0.60f, 0.70f, 0.85f));

    if (m_resetButton) {
        m_resetButton->render(*m_context.spriteRenderer, *m_context.textRenderer);
    }
    if (m_backButton) {
        m_backButton->render(*m_context.spriteRenderer, *m_context.textRenderer);
    }

    // Декоративная разделительная полоса
    m_context.spriteRenderer->drawRect(glm::vec2(80.0f, 70.0f), glm::vec2(1120.0f, 2.0f), glm::vec4(0.35f, 0.55f, 0.95f, 0.5f));

    glm::vec2 mousePos = Input::getMousePosition();

    // -------------------------------------------------------------
    // Левая колонка: Аудио
    // -------------------------------------------------------------
    m_context.textRenderer->renderText(LOC("settings.section_audio"), 115.0f, 95.0f, 0.82f, glm::vec4(0.95f, 0.80f, 0.40f, 1.0f));
    m_context.spriteRenderer->drawRect(glm::vec2(115.0f, 125.0f), glm::vec2(490.0f, 1.0f), glm::vec4(0.35f, 0.45f, 0.60f, 0.35f));

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

        m_context.textRenderer->renderText(LOC(audioSliders[i].labelKey), audioTrackX, y, 0.68f, glm::vec4(0.90f, 0.92f, 0.96f, 1.0f));

        int percent = static_cast<int>(std::round(val * 100.0f));
        std::string valStr = std::to_string(percent) + "%";
        float valW = m_context.textRenderer->measureText(valStr, 0.65f).x;
        m_context.textRenderer->renderText(valStr, audioTrackX + audioTrackW - valW, y, 0.65f, glm::vec4(0.45f, 0.75f, 1.0f, 1.0f));

        float trackY = y + 28.0f;
        m_context.spriteRenderer->drawRect(glm::vec2(audioTrackX, trackY), glm::vec2(audioTrackW, trackH), glm::vec4(0.12f, 0.15f, 0.22f, 1.0f));
        m_context.spriteRenderer->drawRect(glm::vec2(audioTrackX, trackY), glm::vec2(audioTrackW, 1.0f), glm::vec4(0.25f, 0.35f, 0.50f, 0.6f));
        m_context.spriteRenderer->drawRect(glm::vec2(audioTrackX, trackY + trackH - 1.0f), glm::vec2(audioTrackW, 1.0f), glm::vec4(0.25f, 0.35f, 0.50f, 0.6f));

        float fillW = audioTrackW * val;
        if (fillW > 0.0f) {
            m_context.spriteRenderer->drawRect(glm::vec2(audioTrackX, trackY), glm::vec2(fillW, trackH), glm::vec4(0.25f, 0.55f, 0.95f, 1.0f));
        }

        float handleW = 16.0f;
        float handleH = 22.0f;
        float handleX = audioTrackX + fillW - handleW * 0.5f;
        float handleY = trackY + trackH * 0.5f - handleH * 0.5f;
        bool isHovered = (mousePos.x >= handleX - 4.0f && mousePos.x <= handleX + handleW + 4.0f &&
                          mousePos.y >= handleY - 4.0f && mousePos.y <= handleY + handleH + 4.0f) ||
                         (m_activeDraggingSlider == i);

        glm::vec4 handleColor = isHovered ? glm::vec4(1.0f, 0.85f, 0.40f, 1.0f) : glm::vec4(0.90f, 0.93f, 0.98f, 1.0f);
        m_context.spriteRenderer->drawRect(glm::vec2(handleX, handleY), glm::vec2(handleW, handleH), handleColor);
        m_context.spriteRenderer->drawRect(glm::vec2(handleX + 2.0f, handleY + 2.0f), glm::vec2(handleW - 4.0f, handleH - 4.0f), glm::vec4(0.10f, 0.14f, 0.22f, 1.0f));
    }

    // -------------------------------------------------------------
    // Правая колонка: Геймплей и Экран
    // -------------------------------------------------------------
    m_context.textRenderer->renderText(LOC("settings.section_gameplay"), 675.0f, 95.0f, 0.82f, glm::vec4(0.95f, 0.80f, 0.40f, 1.0f));
    m_context.spriteRenderer->drawRect(glm::vec2(675.0f, 125.0f), glm::vec2(480.0f, 1.0f), glm::vec4(0.35f, 0.45f, 0.60f, 0.35f));

    // Слайдер скорости текста (0.005s .. 0.060s)
    float textSpeedVal = Config::getFloat("story.text_speed", 0.030f);
    float textT = (0.060f - textSpeedVal) / (0.060f - 0.005f);
    textT = std::clamp(textT, 0.0f, 1.0f);

    m_context.textRenderer->renderText(LOC("settings.text_speed"), 675.0f, 145.0f, 0.68f, glm::vec4(0.90f, 0.92f, 0.96f, 1.0f));
    int speedPercent = static_cast<int>(std::round(textT * 100.0f));
    std::string speedStr = std::to_string(speedPercent) + "%";
    float speedW = m_context.textRenderer->measureText(speedStr, 0.65f).x;
    m_context.textRenderer->renderText(speedStr, 675.0f + 480.0f - speedW, 145.0f, 0.65f, glm::vec4(0.45f, 0.75f, 1.0f, 1.0f));

    float textTrackY = 145.0f + 28.0f;
    m_context.spriteRenderer->drawRect(glm::vec2(675.0f, textTrackY), glm::vec2(480.0f, trackH), glm::vec4(0.12f, 0.15f, 0.22f, 1.0f));
    m_context.spriteRenderer->drawRect(glm::vec2(675.0f, textTrackY), glm::vec2(480.0f, 1.0f), glm::vec4(0.25f, 0.35f, 0.50f, 0.6f));
    m_context.spriteRenderer->drawRect(glm::vec2(675.0f, textTrackY + trackH - 1.0f), glm::vec2(480.0f, 1.0f), glm::vec4(0.25f, 0.35f, 0.50f, 0.6f));

    float textFillW = 480.0f * textT;
    if (textFillW > 0.0f) {
        m_context.spriteRenderer->drawRect(glm::vec2(675.0f, textTrackY), glm::vec2(textFillW, trackH), glm::vec4(0.25f, 0.55f, 0.95f, 1.0f));
    }
    float textHandleW = 16.0f;
    float textHandleH = 22.0f;
    float textHandleX = 675.0f + textFillW - textHandleW * 0.5f;
    float textHandleY = textTrackY + trackH * 0.5f - textHandleH * 0.5f;
    bool isTextHandleHovered = (mousePos.x >= textHandleX - 4.0f && mousePos.x <= textHandleX + textHandleW + 4.0f &&
                                mousePos.y >= textHandleY - 4.0f && mousePos.y <= textHandleY + textHandleH + 4.0f) ||
                               (m_activeDraggingSlider == 5);
    glm::vec4 textHandleColor = isTextHandleHovered ? glm::vec4(1.0f, 0.85f, 0.40f, 1.0f) : glm::vec4(0.90f, 0.93f, 0.98f, 1.0f);
    m_context.spriteRenderer->drawRect(glm::vec2(textHandleX, textHandleY), glm::vec2(textHandleW, textHandleH), textHandleColor);
    m_context.spriteRenderer->drawRect(glm::vec2(textHandleX + 2.0f, textHandleY + 2.0f), glm::vec2(textHandleW - 4.0f, textHandleH - 4.0f), glm::vec4(0.10f, 0.14f, 0.22f, 1.0f));

    // Выбор языка
    m_context.textRenderer->renderText(LOC("settings.language"), 675.0f, 235.0f, 0.68f, glm::vec4(0.90f, 0.92f, 0.96f, 1.0f));
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

        m_context.spriteRenderer->drawRect(glm::vec2(bx, langBtnY), glm::vec2(langBtnW, langBtnH), btnBg);
        m_context.spriteRenderer->drawRect(glm::vec2(bx, langBtnY), glm::vec2(langBtnW, 1.5f), btnBorder);
        m_context.spriteRenderer->drawRect(glm::vec2(bx, langBtnY + langBtnH - 1.5f), glm::vec2(langBtnW, 1.5f), btnBorder);
        m_context.spriteRenderer->drawRect(glm::vec2(bx, langBtnY), glm::vec2(1.5f, langBtnH), btnBorder);
        m_context.spriteRenderer->drawRect(glm::vec2(bx + langBtnW - 1.5f, langBtnY), glm::vec2(1.5f, langBtnH), btnBorder);

        std::string label = (langs[i] == "ru" ? "Русский" : (langs[i] == "en" ? "English" : "Українська"));
        float txtW = m_context.textRenderer->measureText(label, 0.62f).x;
        glm::vec4 txtCol = isCur ? glm::vec4(1.0f, 0.95f, 0.80f, 1.0f) : glm::vec4(0.80f, 0.85f, 0.92f, 0.9f);
        m_context.textRenderer->renderText(label, bx + (langBtnW - txtW) * 0.5f, langBtnY + 8.0f, 0.62f, txtCol);
    }

    // Тумблеры (VSync, Fullscreen, Auto-dim)
    struct ToggleDef {
        std::string labelKey;
        bool state;
        float y;
    };
    ToggleDef toggles[3] = {
        {"settings.vsync",      Config::getBool("window.vsync", true),                                                          340.0f},
        {"settings.fullscreen", m_context.window ? m_context.window->isFullscreen() : Config::getBool("window.fullscreen", false), 420.0f},
        {"settings.auto_dim",   Config::getBool("story.auto_dim_inactive", false),                                              500.0f}
    };

    for (const auto& tog : toggles) {
        m_context.textRenderer->renderText(LOC(tog.labelKey), 675.0f, tog.y + 6.0f, 0.68f, glm::vec4(0.90f, 0.92f, 0.96f, 1.0f));

        float togX = 1045.0f;
        float togW = 100.0f;
        float togH = 34.0f;
        bool isHover = (mousePos.x >= togX && mousePos.x <= togX + togW &&
                        mousePos.y >= tog.y && mousePos.y <= tog.y + togH);

        glm::vec4 bgCol = tog.state ? (isHover ? glm::vec4(0.20f, 0.52f, 0.30f, 0.95f) : glm::vec4(0.15f, 0.42f, 0.24f, 0.90f))
                                    : (isHover ? glm::vec4(0.22f, 0.15f, 0.17f, 0.95f) : glm::vec4(0.16f, 0.11f, 0.13f, 0.90f));
        glm::vec4 brdCol = tog.state ? glm::vec4(0.35f, 0.88f, 0.50f, 1.0f) : glm::vec4(0.65f, 0.30f, 0.35f, 0.8f);

        m_context.spriteRenderer->drawRect(glm::vec2(togX, tog.y), glm::vec2(togW, togH), bgCol);
        m_context.spriteRenderer->drawRect(glm::vec2(togX, tog.y), glm::vec2(togW, 1.5f), brdCol);
        m_context.spriteRenderer->drawRect(glm::vec2(togX, tog.y + togH - 1.5f), glm::vec2(togW, 1.5f), brdCol);
        m_context.spriteRenderer->drawRect(glm::vec2(togX, tog.y), glm::vec2(1.5f, togH), brdCol);
        m_context.spriteRenderer->drawRect(glm::vec2(togX + togW - 1.5f, tog.y), glm::vec2(1.5f, togH), brdCol);

        std::string txt = tog.state ? LOC("settings.on") : LOC("settings.off");
        float tw = m_context.textRenderer->measureText(txt, 0.62f).x;
        glm::vec4 textCol = tog.state ? glm::vec4(0.85f, 1.0f, 0.85f, 1.0f) : glm::vec4(0.95f, 0.70f, 0.75f, 0.9f);
        m_context.textRenderer->renderText(txt, togX + (togW - tw) * 0.5f, tog.y + 7.0f, 0.62f, textCol);
    }
}
