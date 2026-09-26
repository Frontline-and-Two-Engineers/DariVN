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
