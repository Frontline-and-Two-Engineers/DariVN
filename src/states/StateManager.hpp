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

#ifndef DARIVN_STATEMANAGER_HPP
#define DARIVN_STATEMANAGER_HPP

#include "IGameState.hpp"
#include <vector>
#include <memory>

class StateManager {
public:
    explicit StateManager(SharedContext& context);
    ~StateManager();

    void pushState(std::unique_ptr<IGameState> state);
    void popState();
    void changeState(std::unique_ptr<IGameState> state);
    void clearStates();

    void handleInput();
    void update(float dt);
    void render();

    [[nodiscard]] bool isEmpty() const { return m_stateStack.empty(); }
    [[nodiscard]] size_t getStateCount() const { return m_stateStack.size(); }
    [[nodiscard]] IGameState* getCurrentState() const;
    [[nodiscard]] StateType getCurrentStateType() const;

private:
    void applyPendingChanges();

    enum class PendingActionType {
        Push,
        Pop,
        Change,
        Clear
    };

    struct PendingChange {
        PendingActionType type;
        std::unique_ptr<IGameState> state;
    };

    SharedContext& m_context;
    std::vector<std::unique_ptr<IGameState>> m_stateStack;
    std::vector<PendingChange> m_pendingChanges;
};

#endif // DARIVN_STATEMANAGER_HPP
