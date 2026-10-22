#pragma once
#include <cstdint>
#include "../../../bsp/audio/Speaker_Class.hpp"
class DEVICES;

/**
 * Dino game non-blocking SFX player.
 *
 * Runs a FreeRTOS task on core 1 that decodes the ESPboyPlaytune
 * score format from sounds.h and streams faded PCM to Speaker_Class.
 * All calls here are non-blocking and safe from the game loop on core 0.
 *
 * ESPboyPlaytune byte format (subset supported here — channel 0 only):
 *   0x90 note [hi lo]  Note ON  ch0, MIDI note, optional delay = hi*256+lo ms
 *   0x80      [hi lo]  Note OFF ch0, optional delay
 *   0xf0               End of SFX (stop)
 *   0xe0               End of score (loop — treated as stop for SFX)
 * Delay bytes are present only when the next byte is < 0x80.
 */
void dino_sfx_init(DEVICES* device);
void dino_sfx_deinit();

/** Queue an SFX score; replaces the previous score after its current note. */
void dino_sfx_play(const uint8_t* score);

/** Cancel queued playback and stop after the current note. */
void dino_sfx_stop();
