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
