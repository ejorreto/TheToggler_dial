#include "soundManager.h"

SoundManager::SoundManager() {
    // Constructor - all sounds initialized as unused through Sound default constructor
}

SoundManager::sound_status_t SoundManager::registerSound(float frequency_hz, 
                                                       uint32_t duration_ms, 
                                                       uint8_t& soundId) {
    uint8_t slot = findFreeSlot();
    if (slot >= MAX_SOUNDS) {
        return SOUND_REGISTER_FULL;
    }

    sounds[slot] = Sound(frequency_hz, duration_ms);
    soundId = slot;
    return SOUND_OK;
}

SoundManager::sound_status_t SoundManager::playSound(uint8_t soundId) const {
    if (!isValidSoundId(soundId)) {
        return SOUND_NOT_FOUND;
    }

    M5Dial.Speaker.tone(sounds[soundId].frequency_hz, sounds[soundId].duration_ms);
    return SOUND_OK;
}

SoundManager::sound_status_t SoundManager::unregisterSound(uint8_t soundId) {
    if (!isValidSoundId(soundId)) {
        return SOUND_NOT_FOUND;
    }

    sounds[soundId].used = false;
    return SOUND_OK;
}

uint8_t SoundManager::findFreeSlot() const {
    for (uint8_t i = 0U; i < MAX_SOUNDS; i++) {
        if (!sounds[i].used) {
            return i;
        }
    }
    return MAX_SOUNDS;
}

bool SoundManager::isValidSoundId(uint8_t soundId) const {
    return (soundId < MAX_SOUNDS) && sounds[soundId].used;
}