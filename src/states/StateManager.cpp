#include "StateManager.hpp"

#include "core/Window.hpp"
#include "renderer/RenderPipeline.hpp"
#include "script/StoryEngine.hpp"

#include <glad/glad.h>
#include <algorithm>

StateManager::StateManager(SharedContext& context)
    : m_context(context) {
}

StateManager::~StateManager() {
    clearStates();
    applyPendingChanges();
}

void StateManager::pushState(std::unique_ptr<IGameState> state) {
    m_pendingChanges.push_back({PendingActionType::Push, std::move(state)});
}

void StateManager::popState() {
    m_pendingChanges.push_back({PendingActionType::Pop, nullptr});
}

void StateManager::changeState(std::unique_ptr<IGameState> state) {
    m_pendingChanges.push_back({PendingActionType::Change, std::move(state)});
}

void StateManager::clearStates() {
    m_pendingChanges.push_back({PendingActionType::Clear, nullptr});
}

void StateManager::applyPendingChanges() {
    if (m_pendingChanges.empty()) return;

    for (auto& change : m_pendingChanges) {
        switch (change.type) {
            case PendingActionType::Push: {
                if (!m_stateStack.empty()) {
                    m_stateStack.back()->onPause();
                }
                m_stateStack.push_back(std::move(change.state));
                m_stateStack.back()->onEnter();
                break;
            }
            case PendingActionType::Pop: {
                if (!m_stateStack.empty()) {
                    m_stateStack.back()->onExit();
                    m_stateStack.pop_back();
                    if (!m_stateStack.empty()) {
                        m_stateStack.back()->onResume();
                    }
                }
                break;
            }
            case PendingActionType::Change: {
                while (!m_stateStack.empty()) {
                    m_stateStack.back()->onExit();
                    m_stateStack.pop_back();
                }
                m_stateStack.push_back(std::move(change.state));
                m_stateStack.back()->onEnter();
                break;
            }
            case PendingActionType::Clear: {
                while (!m_stateStack.empty()) {
                    m_stateStack.back()->onExit();
                    m_stateStack.pop_back();
                }
                break;
            }
        }
    }
    m_pendingChanges.clear();
}

void StateManager::handleInput() {
    applyPendingChanges();
    if (!m_stateStack.empty()) {
        m_stateStack.back()->handleInput();
    }
}

void StateManager::update(float dt) {
    applyPendingChanges();
    if (!m_stateStack.empty()) {
        m_stateStack.back()->update(dt);
    }
}

void StateManager::render() {
    applyPendingChanges();
    if (m_stateStack.empty()) return;

    IGameState* topState = m_stateStack.back().get();

    // Если верхнее состояние — оверлей (меню паузы, настройки, бэклог, слоты),
    // рендерим под ним базовое состояние через шейдер размытия
    if (topState->isTransparent() && m_stateStack.size() >= 2) {
        IGameState* baseState = nullptr;
        for (int i = static_cast<int>(m_stateStack.size()) - 2; i >= 0; --i) {
            if (!m_stateStack[i]->isTransparent()) {
                baseState = m_stateStack[i].get();
                break;
            }
        }
        if (!baseState) {
            baseState = m_stateStack.front().get();
        }

        if (m_context.renderPipeline && m_context.window) {
            int fbW = 1280, fbH = 720;
            m_context.window->getFramebufferSize(&fbW, &fbH);

            m_context.renderPipeline->beginScene();
            baseState->render();
            m_context.renderPipeline->endScene();

            PostProcessSettings pp = m_context.storyEngine ? m_context.storyEngine->getPostProcessSettings() : PostProcessSettings{};
            pp.blurStrength = std::max(pp.blurStrength, 1.4f);
            pp.vignetteIntensity = std::max(pp.vignetteIntensity, 0.45f);
            m_context.renderPipeline->renderToScreen(pp, fbW, fbH);

            glViewport(0, 0, fbW, fbH);
            topState->render();
        } else {
            baseState->render();
            topState->render();
        }
    } else {
        if (m_context.renderPipeline && topState->getType() == StateType::Gameplay && m_context.window) {
            int fbW = 1280, fbH = 720;
            m_context.window->getFramebufferSize(&fbW, &fbH);

            m_context.renderPipeline->beginScene();
            topState->render();
            m_context.renderPipeline->endScene();

            PostProcessSettings pp = m_context.storyEngine ? m_context.storyEngine->getPostProcessSettings() : PostProcessSettings{};
            m_context.renderPipeline->renderToScreen(pp, fbW, fbH);
        } else {
            topState->render();
        }
    }
}

IGameState* StateManager::getCurrentState() const {
    if (m_stateStack.empty()) return nullptr;
    return m_stateStack.back().get();
}

StateType StateManager::getCurrentStateType() const {
    if (m_stateStack.empty()) return StateType::MainMenu;
    return m_stateStack.back()->getType();
}
