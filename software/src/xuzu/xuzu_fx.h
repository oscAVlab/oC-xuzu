// "Particulas": a screensaver that reacts to what the module is playing (oscAVlab).
// The ISR watches the 4 outputs and TR1. A jump up on an output (a trigger or gate) is a burst of
// sparks and its quarter of the screen lights up as a box that closes in; a new note (a step in a CV)
// is a bubble that rises; TR1 gives the whole cloud a kick. Each output has its own column
// (A B C D, left to right); the height is the voltage, shown by a thick bar that moves up and down.
// The more is going on, the more the cloud swirls.
#pragma once
#include <stdint.h>
#include "OC_core.h"
#include "src/drivers/display.h"

namespace xuzu {
namespace fx {

// ---------------------------------------------------------------- ISR side
struct Ev { uint8_t ch, kind, height, strength; };   // kind 0 hit, 1 note, 2 beat
static constexpr int EVQ = 32;
inline volatile uint8_t ev_w = 0;
inline uint8_t ev_r = 0;
inline Ev evq[EVQ];
inline uint16_t last_v[4] = {0, 0, 0, 0}, lo_v[4] = {65535, 65535, 65535, 65535}, hi_v[4] = {0, 0, 0, 0};
inline uint16_t hold[4] = {0, 0, 0, 0};

inline void Push(uint8_t ch, uint8_t kind, uint8_t height, uint8_t strength) {
  const uint8_t w = ev_w, n = (uint8_t)((w + 1) % EVQ);
  if (n == ev_r) return;                 // full: drop it
  evq[w] = {ch, kind, height, strength};
  ev_w = n;
}

// one call per ISR tick, after the app has set the outputs. dac[] = raw DAC codes, beat = TR1 clocked
inline void Sample(const uint32_t *dac, bool beat) {
  for (int c = 0; c < 4; ++c) {
    const uint16_t v = (uint16_t)dac[c];
    // follow the range each output moves in, so the height means something for any patch
    if (v < lo_v[c]) lo_v[c] = v; else if ((OC::CORE::ticks & 63) == 0 && lo_v[c] < v) lo_v[c]++;
    if (v > hi_v[c]) hi_v[c] = v; else if ((OC::CORE::ticks & 63) == 0 && hi_v[c] > v) hi_v[c]--;
    const int d = (int)v - (int)last_v[c];
    last_v[c] = v;
    if (hold[c]) { hold[c]--; continue; }
    const int span = hi_v[c] - lo_v[c] > 2000 ? hi_v[c] - lo_v[c] : 2000;
    const uint8_t h = (uint8_t)((v - lo_v[c]) * 63 / span);
    if (d > 12000) { Push(c, 0, h, (uint8_t)(d / 1600 < 20 ? d / 1600 : 20)); hold[c] = 40; }      // trigger / gate on
    else if ((d > 400 || d < -400) && d < 12000) { Push(c, 1, h, (uint8_t)((d > 0 ? d : -d) / 500)); hold[c] = 60; }   // a new note
  }
  if (beat) Push(0, 2, 32, 1);
}

// ---------------------------------------------------------------- screen side
struct P { int16_t x, y, vx, vy; uint8_t life, kind; };   // position in 1/64 px
static constexpr int NP = 72;
inline P parts[NP];
struct Box { uint8_t step, max; };     // per output: a frame of its column that closes in
inline Box boxes[4];
inline uint32_t rng = 0x0A5C1AB5u;
inline int energy = 0;          // activity, 0..1000
inline int swirl = 0;
inline uint32_t frame = 0;

inline int R(int n) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (int)(rng % (uint32_t)n); }
inline void Spawn(int x, int y, int vx, int vy, int life, int kind) {
  int best = 0, bl = 255;
  for (int i = 0; i < NP; ++i) { if (!parts[i].life) { best = i; bl = 0; break; } if (parts[i].life < bl) { bl = parts[i].life; best = i; } }
  parts[best] = {(int16_t)(x * 64), (int16_t)(y * 64), (int16_t)vx, (int16_t)vy, (uint8_t)life, (uint8_t)kind};
}
inline void AddBox(int ch, int strength) { boxes[ch] = {0, (uint8_t)(6 + strength / 2)}; }

inline void Draw() {
  frame++;
  // what happened since the last frame
  while (ev_r != ev_w) {
    const Ev e = evq[ev_r]; ev_r = (uint8_t)((ev_r + 1) % EVQ);
    const int x = 16 + 32 * e.ch, y = 60 - e.height * 56 / 63;
    if (e.kind == 0) {                      // hit: a fountain of sparks from low in its column, and a ring
      const int n = 6 + e.strength / 2, hy = 46;
      for (int i = 0; i < n; ++i) Spawn(x, hy, R(201) - 100, -60 - R(140), 14 + R(14), 0);
      AddBox(e.ch, e.strength);
      energy += 60;
    } else if (e.kind == 1) {               // note: a bubble that floats up
      Spawn(x, y, R(41) - 20, -40 - R(30), 50 + R(30), 1);
      energy += 15;
    } else {                                // TR1: kick the whole cloud
      for (int i = 0; i < NP; ++i) if (parts[i].life) { parts[i].vx += (parts[i].x / 64 < 64 ? -24 : 24); parts[i].vy -= 20; }
      swirl = 1 - swirl;
    }
  }
  if (energy > 1000) energy = 1000;
  if (energy > 0) energy -= 1 + energy / 64;
  // a few drifting motes, more when it is busy
  if (R(1000) < 20 + energy / 4) Spawn(R(128), 63, R(31) - 15, -10 - R(20) - energy / 40, 60 + R(60), 2);

  for (int i = 0; i < NP; ++i) {
    P &p = parts[i];
    if (!p.life) continue;
    p.life--;
    if (p.kind == 0) { p.vy += 10; p.vx = p.vx * 15 / 16; }                 // sparks: gravity and drag
    else if (p.kind == 1) { p.vx = constrain(p.vx + R(9) - 4, -30, 30); }   // bubbles wobble
    else {                                                                   // motes: they swirl with the energy
      const int dx = p.x / 64 - 64, dy = p.y / 64 - 32;
      const int s = energy / 50 * (swirl ? 1 : -1);
      p.vx += -dy * s / 64; p.vy += dx * s / 64;
      p.vx = constrain(p.vx, -60, 60); p.vy = constrain(p.vy, -80, 60);
    }
    p.x += p.vx; p.y += p.vy;
    const int px = p.x / 64, py = p.y / 64;
    if (px < 0 || px > 127 || py < 0 || py > 63) { p.life = 0; continue; }
    if (p.kind == 1) { if (p.life > 8) graphics.drawFrame(px, py, 3, 3); else graphics.setPixel(px, py); }
    else if (p.kind == 0 && p.life > 10) graphics.drawRect(px, py, 2, 2);
    else graphics.setPixel(px, py);
  }
  for (int c = 0; c < 4; ++c) {             // a hit: its quarter of the screen, closing in
    Box &b = boxes[c];
    if (b.step >= b.max) continue;
    const int i = b.step * 2;
    if (i < 15) { graphics.drawFrame(32 * c + 1 + i, i, 30 - 2 * i, 64 - 2 * i); if (b.step == 0) graphics.drawFrame(32 * c + 2, 1, 28, 62); }
    if (frame & 1) b.step++;
  }
  // the 4 outputs as thick bars in their column that go up and down with the voltage
  for (int c = 0; c < 4; ++c) {
    const int span = hi_v[c] - lo_v[c] > 2000 ? hi_v[c] - lo_v[c] : 2000;
    const int h = constrain((int)(last_v[c] - lo_v[c]) * 58 / span, 0, 58);
    graphics.drawRect(32 * c + 6, 60 - h, 20, 3);
  }
}

} // namespace fx
} // namespace xuzu
