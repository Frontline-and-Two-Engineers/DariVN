#pragma once

#ifndef DARIVN_SAVELOADSTATE_HPP
#define DARIVN_SAVELOADSTATE_HPP

#include "IGameState.hpp"
#include "scene/Button.hpp"
#include "core/SaveManager.hpp"
#include <vector>
#include <memory>

class SaveLoadState : public IGameState {
public:
    SaveLoadState(SharedContext& context, bool isSaving);
    ~SaveLoadState() override = default;

    void onEnter() override;
    void handleInput() override;
    void update(float dt) override;
    void render() override;

    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] StateType getType() const override {
        return m_isSaving ? StateType::SaveMenu : StateType::LoadMenu;
    }

    void refreshMetadata();
    void performSave(int slotIndex);
    void performLoad(int slotIndex);

private:
    bool m_isSaving = false;
    std::unique_ptr<Button> m_backButton;
    std::vector<SaveSlotMetadata> m_cachedSlots;
};

#endif // DARIVN_SAVELOADSTATE_HPP
