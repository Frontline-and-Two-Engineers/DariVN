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
