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

#ifndef DARIVN_IGAMESTATE_HPP
#define DARIVN_IGAMESTATE_HPP

#include "SharedContext.hpp"

enum class StateType {
    MainMenu,
    Gameplay,
    PauseMenu,
    Backlog,
    SaveMenu,
    LoadMenu,
    SettingsMenu
};

class IGameState {
public:
    explicit IGameState(SharedContext& context) : m_context(context) {}
    virtual ~IGameState() = default;

    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void onPause() {}
    virtual void onResume() {}

    virtual void handleInput() = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;

    // Оверлеи (PauseMenu, Backlog, SaveLoad, Settings) возвращают true,
    // чтобы нижележащее состояние (например Gameplay) продолжало отрисовываться под ними
    [[nodiscard]] virtual bool isTransparent() const { return false; }
    [[nodiscard]] virtual StateType getType() const = 0;

protected:
    SharedContext& m_context;
};

#endif // DARIVN_IGAMESTATE_HPP
