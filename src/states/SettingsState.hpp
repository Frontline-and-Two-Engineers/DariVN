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

#ifndef DARIVN_SETTINGSSTATE_HPP
#define DARIVN_SETTINGSSTATE_HPP

#include "IGameState.hpp"
#include "scene/Button.hpp"
#include <memory>

class SettingsState : public IGameState {
public:
    explicit SettingsState(SharedContext& context);
    ~SettingsState() override = default;

    void onEnter() override;
    void handleInput() override;
    void update(float dt) override;
    void render() override;

    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] StateType getType() const override { return StateType::SettingsMenu; }

    void initButtons();

private:
    std::unique_ptr<Button> m_backButton;
    std::unique_ptr<Button> m_resetButton;
    int m_activeDraggingSlider = -1; // -1: none, 0..4: audio, 5: text speed
};

#endif // DARIVN_SETTINGSSTATE_HPP
