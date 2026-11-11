#include "Arduino.h"
#include "xuzu/xuzu_fx.h"
#include "xuzu/compas_engine.h"
#include <cstdio>
#include <initializer_list>
// a Techno Compas at 128 bpm on the 4 outputs: A kick, B melody, C fills, D root -> the screensaver
int main(int argc, char **argv) {
  compas::Engine e; e.style = argc > 1 ? atoi(argv[1]) : compas::S_TECHNO; e.Restart(42); e.density = 70; e.notas = 70;
  const int corchea = 16667 * 60 / 128 / 2;
  uint32_t dac[4] = {20000, 20000, 20000, 20000};
  int gate[4] = {0, 0, 0, 0};
  FILE *f = fopen("frames.bin", "wb");
  int pend_mel = -1, pend_t = 0;
  for (int t = 0; t < 16667 * 8; ++t) {
    OC::CORE::ticks++;
    bool beat = false;
    if (t % corchea == 0) {
      compas::Event ev = e.Tick();
      if (ev.fine_a & 1) gate[0] = 170;
      if (ev.fine_b & 1) gate[2] = 170;
      if (ev.fine_b & 8) { pend_t = corchea / 2; }
      if (ev.mel_n) { dac[1] = 20000 + ev.mel_semis[0] * 546; }
      if (ev.mel_n > 1) { pend_mel = 20000 + ev.mel_semis[1] * 546; }
      dac[3] = 20000 + e.RootSemis() * 546;
      beat = ev.beat_start;
    }
    if (t % corchea == corchea / 2) { if (pend_mel >= 0) { dac[1] = pend_mel; pend_mel = -1; } if (pend_t) { gate[2] = 170; pend_t = 0; } }
    for (int c : {0, 2}) { dac[c] = gate[c] > 0 ? 52000 : 20000; if (gate[c] > 0) gate[c]--; }
    xuzu::fx::Sample(dac, beat);
    if (t % 555 == 0) { graphics.clear(); xuzu::fx::Draw(); fwrite(graphics.px, 1, sizeof graphics.px, f); }
  }
  fclose(f);
}
