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

#include "SaveLoadState.hpp"
#include "GameplayState.hpp"
#include "StateManager.hpp"

#include "core/Input.hpp"
#include "resource/ResourceManager.hpp"
#include "resource/Localization.hpp"
#include "renderer/SpriteRenderer.hpp"
#include "renderer/TextRenderer.hpp"
#include "script/StoryEngine.hpp"

#include <GLFW/glfw3.h>

SaveLoadState::SaveLoadState(SharedContext& context, bool isSaving)
    : IGameState(context), m_isSaving(isSaving) {
}

void SaveLoadState::onEnter() {
    m_backButton = std::make_unique<Button>(
        glm::vec2(1080.0f, 18.0f), glm::vec2(120.0f, 44.0f),
        LOC("saveload.back"),
        [this]() {
            m_context.stateManager->popState();
        }
    );
    refreshMetadata();
}

void SaveLoadState::refreshMetadata() {
    m_cachedSlots = SaveManager::getSlotsMetadata(SaveManager::DEFAULT_MAX_SLOTS);
    for (int slot = 1; slot <= SaveManager::DEFAULT_MAX_SLOTS; ++slot) {
        ResourceManager::removeTexture("slot_" + std::to_string(slot));
    }
}

void SaveLoadState::performSave(int slotIndex) {
    if (!m_context.storyEngine) return;

    GameSaveState state;
    m_context.storyEngine->captureSaveState(state);

    std::vector<uint8_t> thumbPixels;
    int tw = 0, th = 0;
    if (m_context.captureScreenThumbnail) {
        m_context.captureScreenThumbnail(thumbPixels, tw, th);
    }

    if (SaveManager::saveSlot(slotIndex, state, thumbPixels.empty() ? nullptr : thumbPixels.data(), tw, th)) {
        refreshMetadata();
        if (m_context.showToast) {
            std::string msg = LOC("saveload.saved_toast") + " " + std::to_string(slotIndex);
            m_context.showToast(msg);
        }
    }
}

void SaveLoadState::performLoad(int slotIndex) {
    GameSaveState state;
    if (!SaveManager::loadSlot(slotIndex, state)) return;

    if (m_context.restoreGameState && m_context.restoreGameState(state)) {
        m_context.stateManager->changeState(std::make_unique<GameplayState>(m_context));
        if (m_context.showToast) {
            m_context.showToast(LOC("saveload.loaded_toast"));
        }
    }
}

void SaveLoadState::handleInput() {
    // 1. Закрытие по Escape или ПКМ
    if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE) || Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        m_context.stateManager->popState();
        return;
    }

    glm::vec2 mousePos = Input::getMousePosition();
    bool mouseClicked = Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);

    // 2. Кнопка "Назад"
    if (m_backButton) {
        m_backButton->update(mousePos, mouseClicked);
    }

    // 3. Обработка клика по карточкам слотов (Слоты 1..6)
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
                    refreshMetadata();
                    if (m_context.showToast) {
                        m_context.showToast(LOC("saveload.deleted_toast"));
                    }
                    break;
                }
            }

            // Клик по карточке слота
            if (mousePos.x >= x && mousePos.x <= x + cardW &&
                mousePos.y >= y && mousePos.y <= y + cardH) {
                if (m_isSaving) {
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
}

void SaveLoadState::update(float dt) {
    (void)dt;
}

void SaveLoadState::render() {
    if (!m_context.spriteRenderer || !m_context.textRenderer) return;

    // 1. Затемнение фона поверх размытия сцены
    m_context.spriteRenderer->drawRect(glm::vec2(0.0f, 0.0f), glm::vec2(1280.0f, 720.0f), glm::vec4(0.04f, 0.05f, 0.08f, 0.94f));

    // 2. Верхняя панель заголовка
    std::string titleStr = m_isSaving ? LOC("saveload.save_title") : LOC("saveload.load_title");
    std::string hintStr = m_isSaving ? LOC("saveload.hint_save") : LOC("saveload.hint_load");
    m_context.textRenderer->renderText(titleStr, 80.0f, 14.0f, 0.95f, glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));
    m_context.textRenderer->renderText(hintStr, 80.0f, 44.0f, 0.58f, glm::vec4(0.55f, 0.60f, 0.70f, 0.85f));

    if (m_backButton) {
        m_backButton->render(*m_context.spriteRenderer, *m_context.textRenderer);
    }

    // Декоративная разделительная полоса
    m_context.spriteRenderer->drawRect(glm::vec2(80.0f, 70.0f), glm::vec2(1087.0f, 2.0f), glm::vec4(0.35f, 0.55f, 0.95f, 0.5f));

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
        m_context.spriteRenderer->drawRect(glm::vec2(x, y), glm::vec2(cardW, cardH), bgColor);

        // Рамка карточки
        glm::vec4 borderColor = isHovered ? glm::vec4(0.95f, 0.65f, 0.25f, 1.0f) : glm::vec4(0.25f, 0.35f, 0.55f, 0.45f);
        float bw = isHovered ? 2.5f : 1.5f;
        m_context.spriteRenderer->drawRect(glm::vec2(x, y), glm::vec2(cardW, bw), borderColor);
        m_context.spriteRenderer->drawRect(glm::vec2(x, y + cardH - bw), glm::vec2(cardW, bw), borderColor);
        m_context.spriteRenderer->drawRect(glm::vec2(x, y), glm::vec2(bw, cardH), borderColor);
        m_context.spriteRenderer->drawRect(glm::vec2(x + cardW - bw, y), glm::vec2(bw, cardH), borderColor);

        // Заголовок слота
        std::string slotTitle = LOC("saveload.slot_prefix") + " " + std::to_string(slot);
        m_context.textRenderer->renderText(slotTitle, x + 16.0f, y + 12.0f, 0.75f, glm::vec4(1.0f, 0.85f, 0.40f, 1.0f));

        if (exists) {
            // Дата и время сохранения
            m_context.textRenderer->renderText(meta->timestamp, x + 120.0f, y + 16.0f, 0.58f, glm::vec4(0.60f, 0.65f, 0.75f, 0.9f));

            // Кнопка удаления (✕)
            bool delHover = (mousePos.x >= x + cardW - 36.0f && mousePos.x <= x + cardW - 12.0f &&
                             mousePos.y >= y + 8.0f && mousePos.y <= y + 32.0f);
            glm::vec4 delColor = delHover ? glm::vec4(0.95f, 0.35f, 0.35f, 1.0f) : glm::vec4(0.55f, 0.60f, 0.70f, 0.65f);
            m_context.textRenderer->renderText("✕", x + cardW - 30.0f, y + 12.0f, 0.75f, delColor);

            // Миниатюра-скриншот
            float thumbX = x + 16.0f;
            float thumbY = y + 42.0f;
            float thumbW = cardW - 32.0f;
            float thumbH = 135.0f;

            if (!meta->screenshotPath.empty()) {
                auto tex = ResourceManager::loadTexture("slot_" + std::to_string(slot), meta->screenshotPath);
                if (tex) {
                    m_context.spriteRenderer->drawSprite(*tex, glm::vec2(thumbX, thumbY), glm::vec2(thumbW, thumbH));
                } else {
                    m_context.spriteRenderer->drawRect(glm::vec2(thumbX, thumbY), glm::vec2(thumbW, thumbH), glm::vec4(0.04f, 0.05f, 0.08f, 0.9f));
                }
            } else {
                m_context.spriteRenderer->drawRect(glm::vec2(thumbX, thumbY), glm::vec2(thumbW, thumbH), glm::vec4(0.04f, 0.05f, 0.08f, 0.9f));
            }

            // Тонкая рамка вокруг миниатюры
            m_context.spriteRenderer->drawRect(glm::vec2(thumbX, thumbY), glm::vec2(thumbW, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));
            m_context.spriteRenderer->drawRect(glm::vec2(thumbX, thumbY + thumbH - 1.0f), glm::vec2(thumbW, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));
            m_context.spriteRenderer->drawRect(glm::vec2(thumbX, thumbY), glm::vec2(1.0f, thumbH), glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));
            m_context.spriteRenderer->drawRect(glm::vec2(thumbX + thumbW - 1.0f, thumbY), glm::vec2(1.0f, thumbH), glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));

            // Превью реплики под скриншотом
            float textY = y + 186.0f;
            if (!meta->previewSpeaker.empty()) {
                m_context.textRenderer->renderText(meta->previewSpeaker, x + 16.0f, textY, 0.65f, glm::vec4(0.95f, 0.80f, 0.40f, 1.0f));
                textY += 22.0f;
            }

            std::string previewText = meta->previewText;
            std::string wrapped = m_context.textRenderer->wrapText(previewText, thumbW, 0.56f);
            m_context.textRenderer->renderText(wrapped, x + 16.0f, textY, 0.56f, glm::vec4(0.85f, 0.88f, 0.92f, 0.9f));
        } else {
            // Пустой слот
            float thumbX = x + 16.0f;
            float thumbY = y + 42.0f;
            float thumbW = cardW - 32.0f;
            float thumbH = 135.0f;

            m_context.spriteRenderer->drawRect(glm::vec2(thumbX, thumbY), glm::vec2(thumbW, thumbH), glm::vec4(0.04f, 0.05f, 0.08f, 0.5f));
            std::string emptyLabel = LOC("saveload.empty");
            float emptyW = m_context.textRenderer->measureText(emptyLabel, 0.70f).x;
            m_context.textRenderer->renderText(emptyLabel, thumbX + (thumbW - emptyW) * 0.5f, thumbY + 54.0f, 0.70f, glm::vec4(0.40f, 0.45f, 0.55f, 0.7f));
        }
    }
}
