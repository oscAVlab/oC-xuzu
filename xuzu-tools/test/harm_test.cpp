// Euclid x4 Acorde / Mel. Compas: the notes follow the chords and the melody of a Compas
#include "xuzu/euclid4_engine.h"
#include <cstdio>
euclid4::Engine euclid_engine;
int main() {
  euclid4::Engine &e = euclid_engine;
  e.Init();
  e.t[2].nota = 1; e.t[2].mel = euclid4::M_ACORDE; e.t[2].hits = 8; e.t[2].steps = 16;
  e.t[3].nota = 1; e.t[3].mel = euclid4::M_COMPAS; e.t[3].hits = 16; e.t[3].steps = 16;
  e.h_style = compas::S_TECHNO;
  int bad = 0, notes2 = 0, notes3 = 0;
  for (int c = 0; c < 16 * 32; ++c) {
    e.Clock16(100);
    const xuzu::Harm h = e.Harmony();
    for (int t = 0; t < 100; ++t) {
      e.Update();
      if (e.hit[2]) { notes2++; int pc = ((e.pitch[2] - 48 - h.root) % 12 + 12) % 12; bool ok = false;
        for (int i = 0; i < h.n; ++i) if (pc == h.tones[i] % 12) ok = true; if (!ok) bad++; }
      if (e.hit[3]) { static int last = -99, ch = 0; if (e.pitch[3] != last) { ch++; last = e.pitch[3]; if (c > 16*31) printf("m%d r%d ", e.pitch[3] - 48, h.root); } notes3++; if (e.pitch[3] != 48 + h.mel) bad++; }
      e.EndTick();
    }
  }
  // following an outside Compas
  xuzu::Harm x; x.root = 5; x.tones[0] = 0; x.tones[1] = 4; x.tones[2] = 7; x.n = 3; x.mel = 17;
  e.ext = &x;
  int ext_bad = 0;
  for (int c = 0; c < 64; ++c) { e.Clock16(100); for (int t = 0; t < 100; ++t) { e.Update();
    if (e.hit[3] && e.pitch[3] != 48 + 17) ext_bad++;
    if (e.hit[2]) { int pc = ((e.pitch[2] - 53) % 12 + 12) % 12; if (pc != 0 && pc != 4 && pc != 7) ext_bad++; }
    e.EndTick(); } }
  printf("acorde %d notas, mel %d notas, fuera %d, externo fuera %d\n", notes2, notes3, bad, ext_bad);
  printf("%s\n", bad || ext_bad || !notes2 || !notes3 ? "FALLA" : "todo bien");
}
