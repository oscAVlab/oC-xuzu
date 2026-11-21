// Just Friends over I2C ("ii", Mannequins), osc[AV]lab v4.25.  Only the o_C T4.1 has the I2C bus (pins 18 SDA / 19 SCL).
// The applets (ISR) only queue messages here; the main loop sends them with Flush(), so the bus never blocks the ISR.
// Wire format (as Teletype / crow): [command, args...], 16-bit args big-endian, pitch/level 1638.3 per volt.
#pragma once
#include <stdint.h>
#include <string.h>
#if defined(ARDUINO_TEENSY41)
#include <Wire.h>
#endif

namespace jf {
enum Cmd : uint8_t { TR = 1, RMODE = 2, RUN = 3, SHIFT = 4, VTR = 5, MODE = 6, TICK = 7, VOX = 8, NOTE = 9, GOD = 10, TUNE = 11, QT = 12 };
constexpr int16_t kVolt = 1638;                        // 1 V in JF units (1638.3)
inline uint8_t Addr(int which) { return which ? 0x75 : 0x70; }   // JF 1 / JF 2 (JF.ADDR 2)

struct Msg { uint8_t addr, n, b[6]; };
inline Msg q[64];
inline volatile uint8_t head = 0, tail = 0;
inline volatile int8_t ok[2] = {-1, -1};               // -1 not checked yet, 0 no answer, 1 answering
inline volatile int8_t want_mode[2] = {-1, -1};        // the mode the applets want: 0 shape, 1 synthesis / geode
inline int8_t sent_mode[2] = {-1, -1};
inline volatile uint32_t sent = 0;                      // messages acknowledged (activity on screen)

// semitones x128 (Hemisphere CV) -> JF pitch: 1536 per volt -> 1638.3 per volt
inline int16_t FromHS(int cv) { return (int16_t)((int32_t)cv * 16383 / 15360); }
inline int16_t Semis(int s) { return (int16_t)((int32_t)s * 16383 / 120); }
inline int16_t Level(int tenths) { return (int16_t)((int32_t)tenths * 16383 / 100); }   // tenths of a volt

inline void Push(uint8_t addr, const uint8_t *b, uint8_t n) {
#if defined(__arm__)
  uint32_t pm; __asm__ volatile("mrs %0, primask" : "=r"(pm)); __asm__ volatile("cpsid i");
#endif
  const uint8_t h = head, nh = (h + 1) & 63;
  if (nh != tail) { q[h].addr = addr; q[h].n = n; memcpy(q[h].b, b, n); head = nh; }
#if defined(__arm__)
  if (!pm) __asm__ volatile("cpsie i");
#endif
}
inline void Send1(int w, uint8_t c, int8_t a) { const uint8_t b[2] = {c, (uint8_t)a}; Push(Addr(w), b, 2); }
inline void Send16(int w, uint8_t c, int16_t v) { const uint8_t b[3] = {c, (uint8_t)(v >> 8), (uint8_t)v}; Push(Addr(w), b, 3); }

// commands
inline void Trigger(int w, int ch, bool on) { const uint8_t b[3] = {TR, (uint8_t)ch, (uint8_t)on}; Push(Addr(w), b, 3); }   // ch 0 = all
inline void VTrigger(int w, int ch, int16_t lvl) { const uint8_t b[4] = {VTR, (uint8_t)ch, (uint8_t)(lvl >> 8), (uint8_t)lvl}; Push(Addr(w), b, 4); }
inline void RunMode(int w, bool on) { Send1(w, RMODE, on); }
inline void Run(int w, int16_t v) { Send16(w, RUN, v); }
inline void Shift(int w, int16_t v) { Send16(w, SHIFT, v); }        // transpose TIME
inline void Tick(int w, int8_t t) { Send1(w, TICK, t); }            // Geode: 1-48 ticks per bar, 49-255 BPM
inline void God(int w, bool on) { Send1(w, GOD, on); }              // A = 432 Hz
inline void Quant(int w, int8_t d) { Send1(w, QT, d); }             // 0 off, else every bar/d
inline void Tune(int w, int ch, int8_t num, int8_t den) { const uint8_t b[4] = {TUNE, (uint8_t)ch, (uint8_t)num, (uint8_t)den}; Push(Addr(w), b, 4); }
// Synthesis: pitch + level.  Geode: divisions + repeats (plain numbers)
inline void Vox(int w, int ch, int16_t a, int16_t v) {
  const uint8_t b[6] = {VOX, (uint8_t)ch, (uint8_t)(a >> 8), (uint8_t)a, (uint8_t)(v >> 8), (uint8_t)v}; Push(Addr(w), b, 6);
}
inline void Note(int w, int16_t a, int16_t v) {
  const uint8_t b[5] = {NOTE, (uint8_t)(a >> 8), (uint8_t)a, (uint8_t)(v >> 8), (uint8_t)v}; Push(Addr(w), b, 5);
}
// the mode is kept by the loop: re-sent when it changes or when a JF shows up again (power cycle)
inline void WantMode(int w, int m) { want_mode[w & 1] = (int8_t)m; }

#if defined(ARDUINO_TEENSY41)
inline bool bus_on = false;
inline void Begin() { Wire.begin(); Wire.setClock(100000); bus_on = true; }
inline uint8_t Tx(uint8_t addr, const uint8_t *b, uint8_t n) { Wire.beginTransmission(addr); if (n) Wire.write(b, n); return Wire.endTransmission(); }
// main loop: send what the applets queued (a few per pass) and check every 2 s which JF answers
inline void Flush() {
  if (!bus_on) return;
  static uint32_t last_probe = 0;
  const uint32_t now = millis();
  for (int w = 0; w < 2; ++w) {
    if (want_mode[w] < 0) continue;
    if (now - last_probe > 2000 || ok[w] < 0) {
      const int8_t was = ok[w];
      ok[w] = Tx(Addr(w), nullptr, 0) == 0;
      if (ok[w] && was != 1) sent_mode[w] = -1;          // just appeared: send it its mode again
    }
    if (ok[w] && sent_mode[w] != want_mode[w]) {
      const uint8_t b[2] = {MODE, (uint8_t)want_mode[w]};
      if (Tx(Addr(w), b, 2) == 0) sent_mode[w] = want_mode[w];
    }
  }
  if (now - last_probe > 2000) last_probe = now;
  for (int i = 0; i < 12 && tail != head; ++i) {
    const Msg &m = q[tail];
    const int w = m.addr == 0x75;
    if (ok[w] != 0) { const uint8_t r = Tx(m.addr, m.b, m.n); if (r == 0) sent++; else ok[w] = 0; }
    tail = (tail + 1) & 63;                               // no JF there: drop it (probe keeps checking)
  }
}
#else
inline void Begin() {}
inline void Flush() { tail = head; }
#endif

// status text for the applets: "JF ok" / "sin JF" / "JF ?"
inline const char *Status(int w) { return ok[w & 1] > 0 ? "JF ok" : ok[w & 1] == 0 ? "sinJF" : "JF ?"; }
} // namespace jf

