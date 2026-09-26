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
