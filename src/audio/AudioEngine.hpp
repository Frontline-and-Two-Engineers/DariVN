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

#ifndef DARIVN_AUDIOENGINE_HPP
#define DARIVN_AUDIOENGINE_HPP

#include <string>
#include <memory>
#include <vector>
#include <mutex>

// Предварительное объявление структур miniaudio для ускорения сборки
struct ma_engine;
struct ma_sound;

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool init();
    void shutdown();
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    // Глобальная регулировка громкости каналов (0.0f - 1.0f)
    void setMasterVolume(float volume);
    void setBGMVolume(float volume);
    void setSFXVolume(float volume);
    void setVoiceVolume(float volume);
    void setAmbientVolume(float volume);

    [[nodiscard]] float getMasterVolume() const { return m_masterVolume; }
    [[nodiscard]] float getBGMVolume() const { return m_bgmVolume; }
    [[nodiscard]] float getSFXVolume() const { return m_sfxVolume; }
    [[nodiscard]] float getVoiceVolume() const { return m_voiceVolume; }
    [[nodiscard]] float getAmbientVolume() const { return m_ambientVolume; }

    // 1. Фоновая музыка (BGM с поддержкой кроссфейда)
    void playMusic(const std::string& filePath, bool loop = true, float fadeDuration = -1.0f, float volume = 1.0f);
    void stopMusic(float fadeDuration = -1.0f);
    void pauseMusic();
    void resumeMusic();
    [[nodiscard]] bool isMusicPlaying() const;
    [[nodiscard]] const std::string& getCurrentMusicPath() const;

    void setDefaultBGMFade(float duration) { m_defaultBGMFade = duration; }
    [[nodiscard]] float getDefaultBGMFade() const { return m_defaultBGMFade; }

    // 2. Звуковые эффекты (SFX)
    void playSound(const std::string& filePath, float volume = 1.0f);

    // 3. Озвучка диалогов (Voice)
    void playVoice(const std::string& filePath, float volume = 1.0f);
    void stopVoice();
    [[nodiscard]] bool isVoicePlaying() const;

    // 4. Фоновое окружение / Эмбиент (Ambient / BGS)
    void playAmbient(const std::string& filePath, bool loop = true, float fadeDuration = 0.0f, float volume = 1.0f);
    void stopAmbient(float fadeDuration = 0.0f);
    [[nodiscard]] bool isAmbientPlaying() const;
    [[nodiscard]] const std::string& getCurrentAmbientPath() const { return m_currentAmbientPath; }

    // 5. Приглушение звука (Music & Ambient Ducking для меню паузы и беклога)
    void setMusicDucked(bool ducked, float duration = 0.4f);
    [[nodiscard]] bool isMusicDucked() const { return m_musicDucked; }
    [[nodiscard]] float getDuckMultiplier() const { return m_currentDuckMultiplier; }

    // Обновление состояния (фейды и очистка завершившихся SFX)
    void update(float deltaTime);

private:
    struct BgmTrack;
    void applyTrackVolume(BgmTrack& track);
    void applyAmbientVolume();
    void applyVoiceVolume();

private:
    bool m_initialized = false;

    float m_masterVolume = 1.0f;
    float m_bgmVolume = 0.8f;
    float m_sfxVolume = 1.0f;
    float m_voiceVolume = 1.0f;
    float m_ambientVolume = 0.8f;
    float m_defaultBGMFade = 1.5f;

    std::unique_ptr<ma_engine> m_engine;

    // BGM (текущий трек и угасающие треки для плавного кроссфейда)
    std::unique_ptr<BgmTrack> m_currentBgm;
    std::vector<std::unique_ptr<BgmTrack>> m_fadingOutBgms;

    // Ambient
    std::unique_ptr<ma_sound> m_ambientSound;
    std::string m_currentAmbientPath;
    float m_ambientUserVolume = 1.0f;
    float m_ambientCurrentFadeGain = 1.0f;
    float m_ambientFadeTimer = 0.0f;
    float m_ambientFadeDuration = 0.0f;
    float m_ambientFadeStartGain = 1.0f;
    float m_ambientFadeTargetGain = 1.0f;
    bool m_ambientFading = false;
    bool m_ambientStopOnFadeComplete = false;

    // Voice
    std::unique_ptr<ma_sound> m_voiceSound;
    float m_voiceUserVolume = 1.0f;

    // Active SFX
    std::vector<std::unique_ptr<ma_sound>> m_activeSfx;
    std::mutex m_sfxMutex;

    // Music & Ambient Ducking
    bool m_musicDucked = false;
    float m_currentDuckMultiplier = 1.0f;
    float m_targetDuckMultiplier = 1.0f;
    float m_duckFadeDuration = 0.4f;
    float m_duckFadeTimer = 0.0f;
    float m_duckStartMultiplier = 1.0f;
};

#endif //DARIVN_AUDIOENGINE_HPP
