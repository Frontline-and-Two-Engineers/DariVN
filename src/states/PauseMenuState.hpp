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

#ifndef DARIVN_PAUSEMENUSTATE_HPP
#define DARIVN_PAUSEMENUSTATE_HPP

#include "IGameState.hpp"
#include "scene/Button.hpp"
#include <vector>

class PauseMenuState : public IGameState {
public:
    explicit PauseMenuState(SharedContext& context);
    ~PauseMenuState() override = default;

    void onEnter() override;
    void onExit() override;
    void onResume() override;
    void handleInput() override;
    void update(float dt) override;
    void render() override;

    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] StateType getType() const override { return StateType::PauseMenu; }

    void initButtons();

private:
    std::vector<Button> m_buttons;
};

#endif // DARIVN_PAUSEMENUSTATE_HPP
