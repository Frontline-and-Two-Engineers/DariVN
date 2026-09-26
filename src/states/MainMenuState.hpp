#pragma once

#ifndef DARIVN_MAINMENUSTATE_HPP
#define DARIVN_MAINMENUSTATE_HPP

#include "IGameState.hpp"
#include "scene/Button.hpp"
#include <vector>

class MainMenuState : public IGameState {
public:
    explicit MainMenuState(SharedContext& context);
    ~MainMenuState() override = default;

    void onEnter() override;
    void onResume() override;
    void handleInput() override;
    void update(float dt) override;
    void render() override;

    [[nodiscard]] bool isTransparent() const override { return false; }
    [[nodiscard]] StateType getType() const override { return StateType::MainMenu; }

    void initButtons();

private:
    std::vector<Button> m_buttons;
};

#endif // DARIVN_MAINMENUSTATE_HPP
