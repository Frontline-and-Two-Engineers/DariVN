#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "AudioEngine.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>

struct AudioEngine::BgmTrack {
    std::unique_ptr<ma_sound> sound;
    std::string path;
    float userVolume = 1.0f;
    float currentFadeGain = 1.0f;
    float fadeStartGain = 1.0f;
    float fadeTargetGain = 1.0f;
    float fadeTimer = 0.0f;
    float fadeDuration = 0.0f;
    bool isFading = false;
    bool stopOnFadeComplete = false;
};

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::init() {
    if (m_initialized) return true;

    m_engine = std::make_unique<ma_engine>();
    ma_result result = ma_engine_init(NULL, m_engine.get());
    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to initialize audio engine: " << result << std::endl;
        m_engine.reset();
        return false;
    }

    m_initialized = true;
    setMasterVolume(m_masterVolume);
    std::cout << "[AudioEngine] Initialized audio engine successfully." << std::endl;
    return true;
}

void AudioEngine::shutdown() {
    if (!m_initialized) return;

    stopMusic(0.0f);
    stopAmbient(0.0f);
    stopVoice();

    {
        std::lock_guard<std::mutex> lock(m_sfxMutex);
        for (auto& sfx : m_activeSfx) {
            if (sfx) {
                ma_sound_stop(sfx.get());
                ma_sound_uninit(sfx.get());
            }
        }
        m_activeSfx.clear();
    }

    if (m_currentBgm) {
        if (m_currentBgm->sound) {
            ma_sound_stop(m_currentBgm->sound.get());
            ma_sound_uninit(m_currentBgm->sound.get());
        }
        m_currentBgm.reset();
    }
    for (auto& track : m_fadingOutBgms) {
        if (track && track->sound) {
            ma_sound_stop(track->sound.get());
            ma_sound_uninit(track->sound.get());
        }
    }
    m_fadingOutBgms.clear();
    if (m_ambientSound) {
        ma_sound_stop(m_ambientSound.get());
        ma_sound_uninit(m_ambientSound.get());
        m_ambientSound.reset();
    }
    if (m_voiceSound) {
        ma_sound_stop(m_voiceSound.get());
        ma_sound_uninit(m_voiceSound.get());
        m_voiceSound.reset();
    }

    if (m_engine) {
        ma_engine_uninit(m_engine.get());
        m_engine.reset();
    }

    m_initialized = false;
    std::cout << "[AudioEngine] Audio engine shut down." << std::endl;
}

void AudioEngine::setMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
    if (m_engine && m_initialized) {
        ma_engine_set_volume(m_engine.get(), m_masterVolume);
    }
}

void AudioEngine::setBGMVolume(float volume) {
    m_bgmVolume = std::clamp(volume, 0.0f, 1.0f);
    if (m_currentBgm) {
        applyTrackVolume(*m_currentBgm);
    }
    for (auto& track : m_fadingOutBgms) {
        if (track) {
            applyTrackVolume(*track);
        }
    }
}

void AudioEngine::applyTrackVolume(BgmTrack& track) {
    if (track.sound && m_initialized) {
        float effectiveVol = m_bgmVolume * track.userVolume * track.currentFadeGain;
        ma_sound_set_volume(track.sound.get(), std::clamp(effectiveVol, 0.0f, 1.0f));
    }
}

void AudioEngine::setAmbientVolume(float volume) {
    m_ambientVolume = std::clamp(volume, 0.0f, 1.0f);
    applyAmbientVolume();
}

void AudioEngine::applyAmbientVolume() {
    if (m_ambientSound && m_initialized) {
        float effectiveVol = m_ambientVolume * m_ambientUserVolume * m_ambientCurrentFadeGain;
        ma_sound_set_volume(m_ambientSound.get(), std::clamp(effectiveVol, 0.0f, 1.0f));
    }
}

void AudioEngine::setVoiceVolume(float volume) {
    m_voiceVolume = std::clamp(volume, 0.0f, 1.0f);
    applyVoiceVolume();
}

void AudioEngine::applyVoiceVolume() {
    if (m_voiceSound && m_initialized) {
        float effectiveVol = m_voiceVolume * m_voiceUserVolume;
        ma_sound_set_volume(m_voiceSound.get(), std::clamp(effectiveVol, 0.0f, 1.0f));
    }
}

void AudioEngine::setSFXVolume(float volume) {
    m_sfxVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioEngine::playMusic(const std::string& filePath, bool loop, float fadeDuration, float volume) {
    if (!m_initialized || filePath.empty()) return;

    float actualFade = (fadeDuration < 0.0f) ? m_defaultBGMFade : fadeDuration;

    // Если этот трек уже играет и не находится в процессе завершения
    if (m_currentBgm && m_currentBgm->path == filePath && !m_currentBgm->stopOnFadeComplete) {
        m_currentBgm->userVolume = volume;
        applyTrackVolume(*m_currentBgm);
        return;
    }

    if (!std::filesystem::exists(filePath)) {
        std::cerr << "[AudioEngine] Music file not found: " << filePath << std::endl;
        return;
    }

    // Ограничиваем количество параллельно угасающих треков при быстрой смене (не более 2)
    while (m_fadingOutBgms.size() > 2) {
        auto& oldTrack = m_fadingOutBgms.front();
        if (oldTrack && oldTrack->sound) {
            ma_sound_stop(oldTrack->sound.get());
            ma_sound_uninit(oldTrack->sound.get());
        }
        m_fadingOutBgms.erase(m_fadingOutBgms.begin());
    }

    // Если сейчас уже играет другой трек — плавно уводим его (Fade Out) параллельно новому
    if (m_currentBgm) {
        if (actualFade > 0.0f) {
            m_currentBgm->isFading = true;
            m_currentBgm->fadeTimer = 0.0f;
            m_currentBgm->fadeDuration = actualFade;
            m_currentBgm->fadeStartGain = m_currentBgm->currentFadeGain;
            m_currentBgm->fadeTargetGain = 0.0f;
            m_currentBgm->stopOnFadeComplete = true;

            m_fadingOutBgms.push_back(std::move(m_currentBgm));
        } else {
            if (m_currentBgm->sound) {
                ma_sound_stop(m_currentBgm->sound.get());
                ma_sound_uninit(m_currentBgm->sound.get());
            }
            m_currentBgm.reset();
        }
    }

    auto newTrack = std::make_unique<BgmTrack>();
    newTrack->sound = std::make_unique<ma_sound>();
    newTrack->path = filePath;
    newTrack->userVolume = volume;

    ma_result result = ma_sound_init_from_file(
        m_engine.get(), 
        filePath.c_str(), 
        MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_ASYNC, 
        NULL, 
        NULL, 
        newTrack->sound.get()
    );

    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to load music: " << filePath << " (error: " << result << ")" << std::endl;
        return;
    }

    ma_sound_set_looping(newTrack->sound.get(), loop ? MA_TRUE : MA_FALSE);

    if (actualFade > 0.0f) {
        newTrack->isFading = true;
        newTrack->fadeTimer = 0.0f;
        newTrack->fadeDuration = actualFade;
        newTrack->fadeStartGain = 0.0f;
        newTrack->fadeTargetGain = 1.0f;
        newTrack->currentFadeGain = 0.0f;
        newTrack->stopOnFadeComplete = false;
    } else {
        newTrack->isFading = false;
        newTrack->currentFadeGain = 1.0f;
    }

    applyTrackVolume(*newTrack);
    ma_sound_start(newTrack->sound.get());
    std::cout << "[AudioEngine] Playing music (crossfade=" << actualFade << "s): " << filePath << (loop ? " (loop)" : "") << std::endl;

    m_currentBgm = std::move(newTrack);
}

void AudioEngine::stopMusic(float fadeDuration) {
    if (!m_initialized) return;

    float actualFade = (fadeDuration < 0.0f) ? m_defaultBGMFade : fadeDuration;

    if (actualFade <= 0.0f) {
        if (m_currentBgm) {
            if (m_currentBgm->sound) {
                ma_sound_stop(m_currentBgm->sound.get());
                ma_sound_uninit(m_currentBgm->sound.get());
            }
            m_currentBgm.reset();
        }
        for (auto& track : m_fadingOutBgms) {
            if (track && track->sound) {
                ma_sound_stop(track->sound.get());
                ma_sound_uninit(track->sound.get());
            }
        }
        m_fadingOutBgms.clear();
    } else {
        if (m_currentBgm) {
            m_currentBgm->isFading = true;
            m_currentBgm->fadeTimer = 0.0f;
            m_currentBgm->fadeDuration = actualFade;
            m_currentBgm->fadeStartGain = m_currentBgm->currentFadeGain;
            m_currentBgm->fadeTargetGain = 0.0f;
            m_currentBgm->stopOnFadeComplete = true;

            m_fadingOutBgms.push_back(std::move(m_currentBgm));
        }
    }
}

void AudioEngine::pauseMusic() {
    if (m_currentBgm && m_currentBgm->sound && m_initialized) {
        ma_sound_stop(m_currentBgm->sound.get());
    }
    for (auto& track : m_fadingOutBgms) {
        if (track && track->sound && m_initialized) {
            ma_sound_stop(track->sound.get());
        }
    }
}

void AudioEngine::resumeMusic() {
    if (m_currentBgm && m_currentBgm->sound && m_initialized) {
        ma_sound_start(m_currentBgm->sound.get());
    }
    for (auto& track : m_fadingOutBgms) {
        if (track && track->sound && m_initialized) {
            ma_sound_start(track->sound.get());
        }
    }
}

bool AudioEngine::isMusicPlaying() const {
    if (m_currentBgm && m_currentBgm->sound && ma_sound_is_playing(m_currentBgm->sound.get())) {
        return true;
    }
    for (const auto& track : m_fadingOutBgms) {
        if (track && track->sound && ma_sound_is_playing(track->sound.get())) {
            return true;
        }
    }
    return false;
}

const std::string& AudioEngine::getCurrentMusicPath() const {
    static const std::string empty;
    return m_currentBgm ? m_currentBgm->path : empty;
}

void AudioEngine::playSound(const std::string& filePath, float volume) {
    if (!m_initialized || filePath.empty() || !std::filesystem::exists(filePath)) return;

    auto sfx = std::make_unique<ma_sound>();
    ma_result result = ma_sound_init_from_file(
        m_engine.get(), 
        filePath.c_str(), 
        MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, 
        NULL, 
        NULL, 
        sfx.get()
    );

    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to play sound: " << filePath << " (error: " << result << ")" << std::endl;
        return;
    }

    float effectiveVol = std::clamp(m_sfxVolume * volume, 0.0f, 1.0f);
    ma_sound_set_volume(sfx.get(), effectiveVol);
    ma_sound_set_looping(sfx.get(), MA_FALSE);
    ma_sound_start(sfx.get());

    std::lock_guard<std::mutex> lock(m_sfxMutex);
    m_activeSfx.push_back(std::move(sfx));
}

void AudioEngine::playVoice(const std::string& filePath, float volume) {
    if (!m_initialized || filePath.empty()) return;

    stopVoice();

    if (!std::filesystem::exists(filePath)) {
        std::cerr << "[AudioEngine] Voice file not found: " << filePath << std::endl;
        return;
    }

    m_voiceSound = std::make_unique<ma_sound>();
    ma_result result = ma_sound_init_from_file(
        m_engine.get(), 
        filePath.c_str(), 
        MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, 
        NULL, 
        NULL, 
        m_voiceSound.get()
    );

    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to play voice: " << filePath << " (error: " << result << ")" << std::endl;
        m_voiceSound.reset();
        return;
    }

    m_voiceUserVolume = volume;
    applyVoiceVolume();
    ma_sound_set_looping(m_voiceSound.get(), MA_FALSE);
    ma_sound_start(m_voiceSound.get());
}

void AudioEngine::stopVoice() {
    if (m_voiceSound && m_initialized) {
        ma_sound_stop(m_voiceSound.get());
        ma_sound_uninit(m_voiceSound.get());
        m_voiceSound.reset();
    }
}

bool AudioEngine::isVoicePlaying() const {
    return m_voiceSound && ma_sound_is_playing(m_voiceSound.get());
}

void AudioEngine::playAmbient(const std::string& filePath, bool loop, float fadeDuration, float volume) {
    if (!m_initialized || filePath.empty()) return;

    if (m_currentAmbientPath == filePath && isAmbientPlaying() && !m_ambientStopOnFadeComplete) {
        m_ambientUserVolume = volume;
        applyAmbientVolume();
        return;
    }

    if (m_ambientSound) {
        ma_sound_stop(m_ambientSound.get());
        ma_sound_uninit(m_ambientSound.get());
        m_ambientSound.reset();
    }

    if (!std::filesystem::exists(filePath)) {
        std::cerr << "[AudioEngine] Ambient file not found: " << filePath << std::endl;
        m_currentAmbientPath.clear();
        return;
    }

    m_ambientSound = std::make_unique<ma_sound>();
    ma_result result = ma_sound_init_from_file(
        m_engine.get(), 
        filePath.c_str(), 
        MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_ASYNC, 
        NULL, 
        NULL, 
        m_ambientSound.get()
    );

    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to play ambient: " << filePath << " (error: " << result << ")" << std::endl;
        m_ambientSound.reset();
        m_currentAmbientPath.clear();
        return;
    }

    m_currentAmbientPath = filePath;
    m_ambientUserVolume = volume;
    ma_sound_set_looping(m_ambientSound.get(), loop ? MA_TRUE : MA_FALSE);

    if (fadeDuration > 0.0f) {
        m_ambientFading = true;
        m_ambientFadeTimer = 0.0f;
        m_ambientFadeDuration = fadeDuration;
        m_ambientFadeStartGain = 0.0f;
        m_ambientFadeTargetGain = 1.0f;
        m_ambientCurrentFadeGain = 0.0f;
        m_ambientStopOnFadeComplete = false;
    } else {
        m_ambientFading = false;
        m_ambientCurrentFadeGain = 1.0f;
    }

    applyAmbientVolume();
    ma_sound_start(m_ambientSound.get());
    std::cout << "[AudioEngine] Playing ambient: " << filePath << (loop ? " (loop)" : "") << std::endl;
}

void AudioEngine::stopAmbient(float fadeDuration) {
    if (!m_ambientSound || !m_initialized) {
        m_currentAmbientPath.clear();
        return;
    }

    if (fadeDuration <= 0.0f) {
        ma_sound_stop(m_ambientSound.get());
        ma_sound_uninit(m_ambientSound.get());
        m_ambientSound.reset();
        m_currentAmbientPath.clear();
        m_ambientFading = false;
    } else {
        m_ambientFading = true;
        m_ambientFadeTimer = 0.0f;
        m_ambientFadeDuration = fadeDuration;
        m_ambientFadeStartGain = m_ambientCurrentFadeGain;
        m_ambientFadeTargetGain = 0.0f;
        m_ambientStopOnFadeComplete = true;
    }
}

bool AudioEngine::isAmbientPlaying() const {
    return m_ambientSound && ma_sound_is_playing(m_ambientSound.get());
}

void AudioEngine::update(float deltaTime) {
    if (!m_initialized) return;

    // 1. Обработка фейдинга текущего BGM (Fade In или затухание)
    if (m_currentBgm && m_currentBgm->isFading) {
        m_currentBgm->fadeTimer += deltaTime;
        float t = (m_currentBgm->fadeDuration > 0.0f) 
            ? std::clamp(m_currentBgm->fadeTimer / m_currentBgm->fadeDuration, 0.0f, 1.0f) 
            : 1.0f;
        m_currentBgm->currentFadeGain = m_currentBgm->fadeStartGain + t * (m_currentBgm->fadeTargetGain - m_currentBgm->fadeStartGain);
        applyTrackVolume(*m_currentBgm);

        if (t >= 1.0f) {
            m_currentBgm->isFading = false;
            if (m_currentBgm->stopOnFadeComplete) {
                if (m_currentBgm->sound) {
                    ma_sound_stop(m_currentBgm->sound.get());
                    ma_sound_uninit(m_currentBgm->sound.get());
                }
                m_currentBgm.reset();
            }
        }
    }

    // 2. Обработка угасающих BGM треков (Fade Out для одновременного кроссфейда)
    for (auto it = m_fadingOutBgms.begin(); it != m_fadingOutBgms.end();) {
        auto& track = *it;
        if (track) {
            track->fadeTimer += deltaTime;
            float t = (track->fadeDuration > 0.0f) 
                ? std::clamp(track->fadeTimer / track->fadeDuration, 0.0f, 1.0f) 
                : 1.0f;
            track->currentFadeGain = track->fadeStartGain + t * (track->fadeTargetGain - track->fadeStartGain);
            applyTrackVolume(*track);

            if (t >= 1.0f) {
                if (track->sound) {
                    ma_sound_stop(track->sound.get());
                    ma_sound_uninit(track->sound.get());
                }
                it = m_fadingOutBgms.erase(it);
                continue;
            }
        }
        ++it;
    }

    // 2. Обработка фейдинга Ambient
    if (m_ambientFading && m_ambientSound) {
        m_ambientFadeTimer += deltaTime;
        float t = std::clamp(m_ambientFadeTimer / m_ambientFadeDuration, 0.0f, 1.0f);
        m_ambientCurrentFadeGain = m_ambientFadeStartGain + t * (m_ambientFadeTargetGain - m_ambientFadeStartGain);
        applyAmbientVolume();

        if (t >= 1.0f) {
            m_ambientFading = false;
            if (m_ambientStopOnFadeComplete) {
                ma_sound_stop(m_ambientSound.get());
                ma_sound_uninit(m_ambientSound.get());
                m_ambientSound.reset();
                m_currentAmbientPath.clear();
                m_ambientStopOnFadeComplete = false;
            }
        }
    }

    // 3. Очистка завершившихся звуков SFX
    {
        std::lock_guard<std::mutex> lock(m_sfxMutex);
        m_activeSfx.erase(
            std::remove_if(m_activeSfx.begin(), m_activeSfx.end(), [](const auto& sound) {
                if (!sound || ma_sound_at_end(sound.get()) || !ma_sound_is_playing(sound.get())) {
                    if (sound) {
                        ma_sound_uninit(sound.get());
                    }
                    return true;
                }
                return false;
            }),
            m_activeSfx.end()
        );
    }
}
