#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "M5Dial.h"

class SoundManager {
public:
    typedef enum {
        SOUND_OK = 0,
        SOUND_REGISTER_FULL = 1,
        SOUND_NOT_FOUND = 2
    } sound_status_t;

    static const uint8_t MAX_SOUNDS = 10;

    struct Sound {
        uint16_t frequency_hz;    // Frequency in Hertz (Hz)
        uint16_t duration_ms;     // Duration in milliseconds (ms)
        bool used;

        Sound() : frequency_hz(0), duration_ms(0), used(false) {}
        Sound(uint16_t freq_hz, uint16_t dur_ms) 
            : frequency_hz(freq_hz), duration_ms(dur_ms), used(true) {}
    };

    SoundManager();
    
    // Register a new sound and get its ID
    sound_status_t registerSound(uint16_t frequency_hz, uint16_t duration_ms, uint8_t& soundId);
    
    // Play a registered sound by its ID
    sound_status_t playSound(uint8_t soundId) const;
    
    // Remove a registered sound
    sound_status_t unregisterSound(uint8_t soundId);

private:
    Sound sounds[MAX_SOUNDS];
    uint8_t findFreeSlot() const;
    bool isValidSoundId(uint8_t soundId) const;
};

#endif // SOUND_MANAGER_H