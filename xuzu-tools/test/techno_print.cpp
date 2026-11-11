#include "xuzu/compas_engine.h"
#include <cstdio>
#include <initializer_list>
int main() {   // prints 4 bars of the Techno line: note names on the 16 semicorcheas, '.' = rest, ! = accent
  static const char *N[12] = {"E","F","F#","G","G#","A","Bb","B","C","C#","D","Eb"};
  for (int seed = 1; seed <= 3; ++seed) for (int dens : {30, 70}) {
    compas::Engine e; e.style = compas::S_TECHNO; e.density = dens; e.Restart(seed * 101);
    printf("semilla %d densidad %d\n", seed, dens);
    for (int bar = 0; bar < 4; ++bar) {
      char line[16][6]; for (auto &l : line) snprintf(l, 6, ".");
      for (int c = 0; c < 8; ++c) { compas::Event ev = e.Tick();
        for (int i = 0; i < ev.mel_n; ++i) { int s = c * 2 + ev.mel_at[i] / 3, v = ev.mel_semis[i];
          snprintf(line[s], 6, "%s%s%s", N[((v % 12) + 12) % 12], v >= 12 ? "'" : "", (ev.mel_acc >> i) & 1 ? "!" : ""); } }
      printf("  "); for (int s = 0; s < 16; ++s) printf("%-4s%s", line[s], (s % 4 == 3) ? "| " : ""); printf("\n");
    }
  }
}
