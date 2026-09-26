#pragma once

#ifndef DARIVN_GAMEPLAYSTATE_HPP
#define DARIVN_GAMEPLAYSTATE_HPP

#include "IGameState.hpp"
#include "scene/Button.hpp"
#include <vector>
#include <glm/glm.hpp>

class GameplayState : public IGameState {
public:
    explicit GameplayState(SharedContext& context);
    ~GameplayState() override = default;

    void onEnter() override;
    void onResume() override;
    void handleInput() override;
    void update(float dt) override;
    void render() override;

    [[nodiscard]] bool isTransparent() const override { return false; }
    [[nodiscard]] StateType getType() const override { return StateType::Gameplay; }

    void updateChoiceButtons();

private:
    std::vector<Button> m_choiceButtons;
    glm::vec2 m_choiceWindowPos{290.0f, 160.0f};
    glm::vec2 m_choiceWindowSize{700.0f, 260.0f};
};

#endif // DARIVN_GAMEPLAYSTATE_HPP
