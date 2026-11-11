// The harmony a Compas plays, for others to follow (Euclid x4's "Acorde" and "Mel. Compas" melodies).
// Pitches in Compas semitones: the same 0 as its CV outputs.
#pragma once
#include <stdint.h>
namespace xuzu {
struct Harm { int8_t root = 0; int8_t tones[4] = {0, 3, 7, -1}; uint8_t n = 3; int8_t mel = 12; };
inline Harm compas_harm;                 // published by the Compas applet every ISR tick
inline uint32_t compas_seen_tick = 0;    // when it last did
}
