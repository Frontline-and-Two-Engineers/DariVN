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
