#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "M5Dial.h"

/**
 * @brief Manages sound effects for the M5Dial device
 * @details Provides a registry of sounds with their frequency and duration,
 *          allowing playback through the M5Dial speaker
 */
class SoundManager {
public:
    /**
     * @brief Status codes for sound operations
     */
    typedef enum {
        SOUND_OK = 0,           /**< Operation completed successfully */
        SOUND_REGISTER_FULL = 1,/**< No more sounds can be registered */
        SOUND_NOT_FOUND = 2     /**< Requested sound ID was not found */
    } sound_status_t;

    static const uint8_t MAX_SOUNDS = 10; /**< Maximum number of sounds that can be registered */

    /**
     * @brief Structure to hold sound parameters
     */
    struct Sound {
        float frequency_hz;      /**< Frequency in Hertz (Hz) */
        uint32_t duration_ms;    /**< Duration in milliseconds (ms) */
        bool used;              /**< Indicates if this slot is in use */

        Sound() : frequency_hz(0.0f), duration_ms(0U), used(false) {}
        Sound(float freq_hz, uint32_t dur_ms) 
            : frequency_hz(freq_hz), duration_ms(dur_ms), used(true) {}
    };

    /**
     * @brief Constructor - initializes the sound registry
     */
    SoundManager();
    
    /**
     * @brief Registers a new sound in the system
     * @param frequency_hz The frequency of the sound in Hertz
     * @param duration_ms The duration of the sound in milliseconds
     * @param[out] soundId The ID assigned to the registered sound
     * @return Status of the registration operation
     */
    sound_status_t registerSound(float frequency_hz, uint32_t duration_ms, uint8_t& soundId);
    
    /**
     * @brief Plays a previously registered sound
     * @param soundId The ID of the sound to play
     * @return Status of the playback operation
     */
    sound_status_t playSound(uint8_t soundId) const;
    
    /**
     * @brief Removes a sound from the registry
     * @param soundId The ID of the sound to unregister
     * @return Status of the unregister operation
     */
    sound_status_t unregisterSound(uint8_t soundId);

private:
    Sound sounds[MAX_SOUNDS];   /**< Array of registered sounds */

    /**
     * @brief Finds the next available slot in the sound registry
     * @return Index of the free slot, or MAX_SOUNDS if none available
     */
    uint8_t findFreeSlot() const;

    /**
     * @brief Checks if a sound ID is valid and in use
     * @param soundId The ID to validate
     * @return true if the ID is valid and in use, false otherwise
     */
    bool isValidSoundId(uint8_t soundId) const;
};

#endif // SOUND_MANAGER_H