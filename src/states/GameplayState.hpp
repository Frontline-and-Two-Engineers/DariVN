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

#pragma once

#ifndef DARIVN_GAMEPLAYSTATE_HPP
#define DARIVN_GAMEPLAYSTATE_HPP

#include "IGameState.hpp"
#include "scene/Button.hpp"
#include <vector>
#include <glm/glm.hpp>

class GameplayState : public IGameState {
public:
    explicit GameplayState(SharedContext& context);
    ~GameplayState() override = default;

    void onEnter() override;
    void onResume() override;
    void handleInput() override;
    void update(float dt) override;
    void render() override;

    [[nodiscard]] bool isTransparent() const override { return false; }
    [[nodiscard]] StateType getType() const override { return StateType::Gameplay; }

    void updateChoiceButtons();
    void initQuickMenuButtons();

private:
    std::vector<Button> m_choiceButtons;
    glm::vec2 m_choiceWindowPos{290.0f, 160.0f};
    glm::vec2 m_choiceWindowSize{700.0f, 260.0f};

    std::vector<Button> m_quickMenuButtons;
    bool m_isSkipActive = false;
    float m_skipTimer = 0.0f;
    float m_skipIndicatorAnim = 0.0f;
    bool m_isUiHidden = false;
};

#endif // DARIVN_GAMEPLAYSTATE_HPP
