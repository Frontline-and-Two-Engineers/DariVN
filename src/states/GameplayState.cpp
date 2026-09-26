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

GameplayState::GameplayState(SharedContext& context)
    : IGameState(context) {
}

void GameplayState::onEnter() {
    if (m_context.storyEngine && m_context.storyEngine->isWaitingForChoice()) {
        updateChoiceButtons();
    }
}

void GameplayState::onResume() {
    if (m_context.storyEngine && m_context.storyEngine->isWaitingForChoice()) {
        updateChoiceButtons();
    }
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

    // 1. Пауза по Escape
    if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE)) {
        m_context.stateManager->pushState(std::make_unique<PauseMenuState>(m_context));
        return;
    }

    // 2. Бэклог по колесику вверх или клавишам L / B / H
    float scrollDelta = Input::getMouseScrollY();
    if (scrollDelta > 0.1f || Input::isKeyJustPressed(GLFW_KEY_L) || Input::isKeyJustPressed(GLFW_KEY_B) || Input::isKeyJustPressed(GLFW_KEY_H)) {
        m_context.stateManager->pushState(std::make_unique<BacklogState>(m_context));
        return;
    }

    // 3. Быстрое сохранение (F5)
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

    // 4. Быстрая загрузка (F9)
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

    // 5. Развилки выбора
    if (m_context.storyEngine && m_context.storyEngine->isWaitingForChoice()) {
        if (m_choiceButtons.empty()) {
            updateChoiceButtons();
        }
        for (auto& btn : m_choiceButtons) {
            btn.update(mousePos, mouseClicked);
        }
        return;
    }

    // 6. Обычное продвижение по диалогу (ЛКМ, Пробел, Enter)
    bool advanceRequested = mouseClicked ||
                            Input::isKeyJustPressed(GLFW_KEY_SPACE) ||
                            Input::isKeyJustPressed(GLFW_KEY_ENTER);

    if (advanceRequested && m_context.storyEngine) {
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
        m_context.storyEngine->update(dt);

        if (m_context.dialogueBox) {
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

    // 3. Отрисовка диалогового окна
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
        m_context.textRenderer->renderText(LOC("gameplay.controls_hint"), 720.0f, 650.0f, 0.65f, glm::vec4(0.5f, 0.55f, 0.65f, 0.7f));
    }
}
