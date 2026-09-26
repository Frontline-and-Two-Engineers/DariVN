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

#ifndef DARIVN_BACKLOGSTATE_HPP
#define DARIVN_BACKLOGSTATE_HPP

#include "IGameState.hpp"
#include "scene/Button.hpp"
#include <memory>

class BacklogState : public IGameState {
public:
    explicit BacklogState(SharedContext& context);
    ~BacklogState() override = default;

    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render() override;

    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] StateType getType() const override { return StateType::Backlog; }

private:
    std::unique_ptr<Button> m_closeButton;
    float m_scrollY = 0.0f;
    float m_maxScrollY = 0.0f;
    bool m_isDraggingScrollbar = false;
    float m_scrollbarDragStartMouseY = 0.0f;
    float m_scrollbarDragStartScrollY = 0.0f;
};

#endif // DARIVN_BACKLOGSTATE_HPP
