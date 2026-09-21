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
};

#endif //DARIVN_AUDIOENGINE_HPP
