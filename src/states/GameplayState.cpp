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

#include "GameplayState.hpp"
#include "PauseMenuState.hpp"
#include "BacklogState.hpp"
#include "SaveLoadState.hpp"
#include "SettingsState.hpp"
#include "StateManager.hpp"

#include "core/Config.hpp"
#include "core/Input.hpp"
#include "core/SaveManager.hpp"
#include "resource/ResourceManager.hpp"
#include "resource/Localization.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"
#include "audio/AudioEngine.hpp"
#include "script/StoryEngine.hpp"
#include "scene/CharacterSprite.hpp"
#include "scene/DialogueBox.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

GameplayState::GameplayState(SharedContext& context)
    : IGameState(context) {
}

void GameplayState::onEnter() {
    initQuickMenuButtons();
    if (m_context.storyEngine && m_context.storyEngine->isWaitingForChoice()) {
        updateChoiceButtons();
    }
}

void GameplayState::onResume() {
    initQuickMenuButtons();
    if (m_context.storyEngine && m_context.storyEngine->isWaitingForChoice()) {
        updateChoiceButtons();
    }
}

void GameplayState::initQuickMenuButtons() {
    m_quickMenuButtons.clear();

    float btnW = 114.0f;
    float btnH = 34.0f;
    float spacing = 8.0f;
    float totalW = 5.0f * btnW + 4.0f * spacing; // 602.0f
    float startX = 1240.0f - totalW; // 638.0f
    float startY = 452.0f;
    float btnFontScale = 0.60f;

    auto addButton = [this, btnFontScale](glm::vec2 pos, glm::vec2 size, const std::string& text, std::function<void()> cb) {
        m_quickMenuButtons.emplace_back(pos, size, text, std::move(cb));
        m_quickMenuButtons.back().setTextScale(btnFontScale);
    };

    // 1. История диалогов (Backlog)
    addButton(
        glm::vec2(startX, startY), glm::vec2(btnW, btnH),
        LOC("quick.history"),
        [this]() {
            m_isSkipActive = false;
            if (m_context.stateManager) {
                m_context.stateManager->pushState(std::make_unique<BacklogState>(m_context));
            }
        }
    );

    // 2. Быстрый пропуск (Skip)
    addButton(
        glm::vec2(startX + 1.0f * (btnW + spacing), startY), glm::vec2(btnW, btnH),
        LOC("quick.skip"),
        [this]() {
            m_isSkipActive = !m_isSkipActive;
        }
    );

    // 3. Сохранить игру (Save)
    addButton(
        glm::vec2(startX + 2.0f * (btnW + spacing), startY), glm::vec2(btnW, btnH),
        LOC("quick.save"),
        [this]() {
            m_isSkipActive = false;
            if (m_context.stateManager) {
                m_context.stateManager->pushState(std::make_unique<SaveLoadState>(m_context, true));
            }
        }
    );

    // 4. Загрузить игру (Load)
    addButton(
        glm::vec2(startX + 3.0f * (btnW + spacing), startY), glm::vec2(btnW, btnH),
        LOC("quick.load"),
        [this]() {
            m_isSkipActive = false;
            if (m_context.stateManager) {
                m_context.stateManager->pushState(std::make_unique<SaveLoadState>(m_context, false));
            }
        }
    );

    // 5. Настройки игры (Settings)
    addButton(
        glm::vec2(startX + 4.0f * (btnW + spacing), startY), glm::vec2(btnW, btnH),
        LOC("quick.settings"),
        [this]() {
            m_isSkipActive = false;
            if (m_context.stateManager) {
                m_context.stateManager->pushState(std::make_unique<SettingsState>(m_context));
            }
        }
    );
}

void GameplayState::updateChoiceButtons() {
    m_choiceButtons.clear();
    if (!m_context.storyEngine) return;
    const auto& choices = m_context.storyEngine->getCurrentChoices();
    if (choices.empty()) return;

    size_t count = choices.size();
    float btnW = 640.0f;
    float btnH = 50.0f;
    float btnSpacing = 14.0f;
    float winW = 700.0f;

    float winH = 68.0f + static_cast<float>(count) * btnH + static_cast<float>(count - 1) * btnSpacing + 18.0f;
    float winX = (1280.0f - winW) * 0.5f;
    float winY = (720.0f - winH) * 0.5f - 20.0f;

    m_choiceWindowPos = glm::vec2(winX, winY);
    m_choiceWindowSize = glm::vec2(winW, winH);

    float btnStartX = winX + (winW - btnW) * 0.5f;
    float btnStartY = winY + 68.0f;

    for (size_t i = 0; i < count; ++i) {
        float y = btnStartY + static_cast<float>(i) * (btnH + btnSpacing);
        size_t choiceIndex = i;

        m_choiceButtons.emplace_back(
            glm::vec2(btnStartX, y),
            glm::vec2(btnW, btnH),
            choices[i].text,
            [this, choiceIndex]() {
                if (m_context.storyEngine) {
                    m_context.storyEngine->chooseOption(choiceIndex);
                }
                m_choiceButtons.clear();
            }
        );
    }
}

void GameplayState::handleInput() {
    glm::vec2 mousePos = Input::getMousePosition();
    bool mouseClicked = Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
    bool rmbClicked = Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT);
    bool mmbClicked = Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_MIDDLE);

    // 1. Если интерфейс скрыт (Hide UI) — любой клик или кнопка возвращают его без продвижения по тексту
    if (m_isUiHidden) {
        if (mouseClicked || rmbClicked || mmbClicked ||
            Input::isKeyJustPressed(GLFW_KEY_H) ||
            Input::isKeyJustPressed(GLFW_KEY_SPACE) ||
            Input::isKeyJustPressed(GLFW_KEY_ENTER) ||
            Input::isKeyJustPressed(GLFW_KEY_ESCAPE)) {
            m_isUiHidden = false;
        }
        return;
    }

    // 2. Скрытие интерфейса (горячая клавиша H или клик средней кнопкой мыши)
    if (Input::isKeyJustPressed(GLFW_KEY_H) || mmbClicked) {
        m_isUiHidden = true;
        return;
    }

    // 3. Пауза по Escape
    if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE)) {
        m_isSkipActive = false;
        m_context.stateManager->pushState(std::make_unique<PauseMenuState>(m_context));
        return;
    }

    // 4. Бэклог по колесику вверх или клавишам L / B
    float scrollDelta = Input::getMouseScrollY();
    if (scrollDelta > 0.1f || Input::isKeyJustPressed(GLFW_KEY_L) || Input::isKeyJustPressed(GLFW_KEY_B)) {
        m_isSkipActive = false;
        m_context.stateManager->pushState(std::make_unique<BacklogState>(m_context));
        return;
    }

    // 5. Быстрое сохранение (F5)
    if (Input::isKeyJustPressed(GLFW_KEY_F5)) {
        if (m_context.storyEngine) {
            GameSaveState state;
            m_context.storyEngine->captureSaveState(state);

            std::vector<uint8_t> thumbPixels;
            int tw = 0, th = 0;
            if (m_context.captureScreenThumbnail) {
                m_context.captureScreenThumbnail(thumbPixels, tw, th);
            }

            if (SaveManager::saveSlot(SaveManager::QUICKSAVE_SLOT_INDEX, state, thumbPixels.empty() ? nullptr : thumbPixels.data(), tw, th)) {
                if (m_context.showToast) {
                    m_context.showToast(LOC("saveload.quicksaved_toast"));
                }
            }
        }
        return;
    }

    // 6. Быстрая загрузка (F9)
    if (Input::isKeyJustPressed(GLFW_KEY_F9)) {
        GameSaveState state;
        if (SaveManager::doesSlotExist(SaveManager::QUICKSAVE_SLOT_INDEX) && SaveManager::loadSlot(SaveManager::QUICKSAVE_SLOT_INDEX, state)) {
            if (m_context.restoreGameState && m_context.restoreGameState(state)) {
                if (m_context.showToast) {
                    m_context.showToast(LOC("saveload.loaded_toast"));
                }
            }
        } else {
            if (m_context.showToast) {
                m_context.showToast(LOC("saveload.no_quicksave"));
            }
        }
        return;
    }

    // 7. Переключение режима пропуска (Tab)
    if (Input::isKeyJustPressed(GLFW_KEY_TAB)) {
        m_isSkipActive = !m_isSkipActive;
    }

    // 8. Развилки выбора
    if (m_context.storyEngine && m_context.storyEngine->isWaitingForChoice()) {
        m_isSkipActive = false; // Отключаем пропуск при выборе
        if (m_choiceButtons.empty()) {
            updateChoiceButtons();
        }
        for (auto& btn : m_choiceButtons) {
            btn.update(mousePos, mouseClicked);
        }
        return;
    }

    // 9. Кнопки быстрого меню (Quick Menu)
    for (auto& btn : m_quickMenuButtons) {
        if (btn.update(mousePos, mouseClicked)) {
            return;
        }
    }

    // 10. Обычное продвижение по диалогу (ЛКМ, Пробел, Enter)
    bool advanceRequested = mouseClicked ||
                            Input::isKeyJustPressed(GLFW_KEY_SPACE) ||
                            Input::isKeyJustPressed(GLFW_KEY_ENTER);

    if (advanceRequested && m_context.storyEngine) {
        if (m_isSkipActive) {
            m_isSkipActive = false;
        }

        if (!m_context.storyEngine->isTextFullyRevealed()) {
            m_context.storyEngine->revealFullText();
        } else {
            m_context.storyEngine->nextStep();
            if (m_context.storyEngine->isWaitingForChoice()) {
                updateChoiceButtons();
            }
        }
    }
}

void GameplayState::update(float dt) {
    if (m_context.storyEngine) {
        // Логика режима пропуска (Skip Mode): удержание Ctrl или включенный тоггл
        bool isCtrlDown = Input::isKeyDown(GLFW_KEY_LEFT_CONTROL) || Input::isKeyDown(GLFW_KEY_RIGHT_CONTROL);
        bool isSkipping = (isCtrlDown || m_isSkipActive) && !m_context.storyEngine->isWaitingForChoice();

        if (isSkipping) {
            m_skipIndicatorAnim += dt * 7.0f;

            if (!m_context.storyEngine->isTextFullyRevealed()) {
                m_context.storyEngine->revealFullText();
                m_skipTimer = 0.0f;
            } else if (m_context.storyEngine->isWaitingForNextClick()) {
                m_skipTimer += dt;
                if (m_skipTimer >= 0.06f) {
                    m_skipTimer = 0.0f;
                    m_context.storyEngine->nextStep();
                    if (m_context.storyEngine->isWaitingForChoice()) {
                        m_isSkipActive = false;
                        updateChoiceButtons();
                    }
                }
            }
        } else {
            m_skipTimer = 0.0f;
        }

        m_context.storyEngine->update(dt);

        if (m_context.dialogueBox && !m_isUiHidden) {
            m_context.dialogueBox->update(dt);
        }

        if (m_context.storyEngine->isWaitingForChoice() && m_choiceButtons.empty()) {
            updateChoiceButtons();
        }

        // 1. Автоматически регистрируем персонажей, определенных в сценарии (define / load)
        for (const auto& [id, def] : m_context.storyEngine->getDefinedCharacters()) {
            if (m_context.getOrCreateCharacter) {
                auto& sprite = m_context.getOrCreateCharacter(id);
                sprite.setDisplayName(def.displayName);
                if (def.hasCustomScale) {
                    sprite.setScale(def.scale);
                }
                for (const auto& [expr, path] : def.expressions) {
                    if (!path.empty()) {
                        std::string resolved = m_context.resolveAsset ? m_context.resolveAsset(path) : path;
                        sprite.addExpression(expr, resolved);
                    }
                }
            }
        }

        // 2. Синхронизируем состояние активных персонажей со StoryEngine (координаты, слой, видимость)
        if (m_context.characters) {
            const auto& activeCharacters = m_context.storyEngine->getActiveCharacters();
            for (auto& [name, character] : *m_context.characters) {
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
                    if (Config::getBool("story.auto_dim_inactive", false) && !m_context.storyEngine->getCurrentSpeaker().empty()) {
                        bool isSpeaker = (character->getDisplayName() == m_context.storyEngine->getCurrentSpeaker() || name == m_context.storyEngine->getCurrentSpeaker());
                        if (!isSpeaker) {
                            character->setTint(it->second.tint * glm::vec4(0.72f, 0.72f, 0.76f, 1.0f));
                        }
                    }

                    character->fadeIn();
                } else {
                    character->fadeOut();
                }
                character->update(dt);
            }
        }
    }
}

void GameplayState::render() {
    if (!m_context.spriteRenderer || !m_context.textRenderer || !m_context.storyEngine) return;

    // 1. Игровой фон
    const std::string& bgPath = m_context.storyEngine->getCurrentBackground();
    if (!bgPath.empty()) {
        std::string resolved = m_context.resolveAsset ? m_context.resolveAsset(bgPath) : bgPath;
        auto bgTex = ResourceManager::loadTexture(bgPath, resolved);
        if (bgTex) {
            m_context.spriteRenderer->drawSprite(*bgTex, glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f));
        } else {
            m_context.spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.11f, 0.13f, 0.20f, 1.0f));
        }
    } else {
        m_context.spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.11f, 0.13f, 0.20f, 1.0f));
    }

    // 2. Отрисовка персонажей с сортировкой по слоям (Z-index / Layer)
    if (m_context.characters) {
        std::vector<CharacterSprite*> sortedCharacters;
        for (auto& [name, character] : *m_context.characters) {
            if (character->isVisible()) {
                sortedCharacters.push_back(character.get());
            }
        }

        std::stable_sort(sortedCharacters.begin(), sortedCharacters.end(), [](const auto* a, const auto* b) {
            return a->getLayer() < b->getLayer();
        });

        for (auto* character : sortedCharacters) {
            character->render(*m_context.spriteRenderer, *m_context.textRenderer);
        }
    }

    // 3. Отрисовка диалогового окна, кнопок и подсказок (если интерфейс не скрыт игроком)
    if (!m_isUiHidden) {
        if (m_context.dialogueBox) {
            bool isWaitingClick = m_context.storyEngine->isWaitingForNextClick();
            m_context.dialogueBox->render(
                *m_context.spriteRenderer,
                *m_context.textRenderer,
                m_context.storyEngine->getCurrentSpeaker(),
                m_context.storyEngine->getVisibleText(),
                isWaitingClick
            );
        }

        // Кнопки быстрого меню (Quick Menu)
        if (!m_context.storyEngine->isWaitingForChoice()) {
            for (auto& btn : m_quickMenuButtons) {
                btn.render(*m_context.spriteRenderer, *m_context.textRenderer);
            }
        }

        // 4. Меню выбора (Choice Menu)
        if (m_context.storyEngine->isWaitingForChoice() && !m_choiceButtons.empty()) {
            m_context.spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.02f, 0.03f, 0.06f, 0.65f));

            m_context.spriteRenderer->drawRect(m_choiceWindowPos, m_choiceWindowSize, glm::vec4(0.08f, 0.10f, 0.16f, 0.95f));

            float bw = 2.0f;
            glm::vec4 borderColor(0.40f, 0.65f, 1.0f, 0.75f);
            m_context.spriteRenderer->drawRect(m_choiceWindowPos, glm::vec2(m_choiceWindowSize.x, bw), borderColor);
            m_context.spriteRenderer->drawRect(glm::vec2(m_choiceWindowPos.x, m_choiceWindowPos.y + m_choiceWindowSize.y - bw), glm::vec2(m_choiceWindowSize.x, bw), borderColor);
            m_context.spriteRenderer->drawRect(m_choiceWindowPos, glm::vec2(bw, m_choiceWindowSize.y), borderColor);
            m_context.spriteRenderer->drawRect(glm::vec2(m_choiceWindowPos.x + m_choiceWindowSize.x - bw, m_choiceWindowPos.y), glm::vec2(bw, m_choiceWindowSize.y), borderColor);

            std::string title = LOC("gameplay.choice_prompt");
            float titleW = m_context.textRenderer->measureText(title, 0.85f).x;
            float titleX = m_choiceWindowPos.x + (m_choiceWindowSize.x - titleW) * 0.5f;
            m_context.textRenderer->renderText(title, titleX, m_choiceWindowPos.y + 18.0f, 0.85f, glm::vec4(1.0f, 0.90f, 0.70f, 1.0f));

            m_context.spriteRenderer->drawRect(
                glm::vec2(m_choiceWindowPos.x + 20.0f, m_choiceWindowPos.y + 52.0f),
                glm::vec2(m_choiceWindowSize.x - 40.0f, 1.5f),
                glm::vec4(0.40f, 0.65f, 1.0f, 0.4f)
            );

            for (auto& btn : m_choiceButtons) {
                btn.render(*m_context.spriteRenderer, *m_context.textRenderer);
            }
        } else {
            // Подсказка управления из локализации
            m_context.textRenderer->renderText(LOC("gameplay.controls_hint"), 40.0f, 692.0f, 0.55f, glm::vec4(0.45f, 0.50f, 0.60f, 0.75f));
        }

        // Индикатор активного режима пропуска (Skip Mode)
        bool isCtrlDown = Input::isKeyDown(GLFW_KEY_LEFT_CONTROL) || Input::isKeyDown(GLFW_KEY_RIGHT_CONTROL);
        if ((isCtrlDown || m_isSkipActive) && !m_context.storyEngine->isWaitingForChoice()) {
            float alpha = 0.72f + 0.28f * std::sin(m_skipIndicatorAnim);
            std::string skipText = LOC("gameplay.skip_indicator");
            float skipScale = 0.72f;
            glm::vec2 skipSize = m_context.textRenderer->measureText(skipText, skipScale);
            float padX = 16.0f, padY = 7.0f;
            float badgeW = skipSize.x + padX * 2.0f;
            float badgeH = skipSize.y + padY * 2.0f;
            float badgeX = 1240.0f - badgeW;
            float badgeY = 24.0f;

            m_context.spriteRenderer->drawRect(glm::vec2(badgeX, badgeY), glm::vec2(badgeW, badgeH), glm::vec4(0.08f, 0.12f, 0.22f, 0.92f * alpha));
            m_context.spriteRenderer->drawRect(glm::vec2(badgeX, badgeY), glm::vec2(badgeW, 2.0f), glm::vec4(0.95f, 0.75f, 0.20f, alpha));
            m_context.spriteRenderer->drawRect(glm::vec2(badgeX, badgeY + badgeH - 2.0f), glm::vec2(badgeW, 2.0f), glm::vec4(0.95f, 0.75f, 0.20f, alpha));
            m_context.spriteRenderer->drawRect(glm::vec2(badgeX, badgeY), glm::vec2(2.0f, badgeH), glm::vec4(0.95f, 0.75f, 0.20f, alpha));
            m_context.spriteRenderer->drawRect(glm::vec2(badgeX + badgeW - 2.0f, badgeY), glm::vec2(2.0f, badgeH), glm::vec4(0.95f, 0.75f, 0.20f, alpha));

            m_context.textRenderer->renderText(skipText, badgeX + padX, badgeY + padY + 2.0f, skipScale, glm::vec4(1.0f, 0.85f, 0.30f, alpha));
        }
    }
}
