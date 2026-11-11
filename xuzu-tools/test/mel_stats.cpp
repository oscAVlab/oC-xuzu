#include "xuzu/compas_engine.h"
#include <cstdio>
#include <cstdlib>
int main() {
  printf("%-12s %6s %6s %6s %6s %6s %6s %6s %6s\n", "estilo", "notas/c", "repite", "baja", "sube", "salto", "octava", "altura", "acor%");
  for (int st = 0; st < compas::NUM_STYLES; ++st) {
    long notes = 0, rep = 0, dn = 0, up = 0, leap = 0, oct = 0, height = 0, cycles = 0, acc = 0, tie = 0; int last = -999;
    for (int seed = 1; seed <= 20; ++seed) {
      compas::Engine e; e.style = st; e.Restart(seed * 7919); last = -999;
      for (int i = 0; i < 400; ++i) {
        compas::Event ev = e.Tick(); if (ev.cycle_start) cycles++;
        for (int k = 0; k < ev.mel_n; ++k) {
          if (!((ev.mel_notes >> k) & 1)) continue;
          const int n = ev.mel_semis[k]; notes++; height += n;
          acc += (ev.mel_acc >> k) & 1; tie += (ev.mel_tie >> k) & 1;
          if (last != -999) { const int d = n - last; if (!d) rep++; else if (d < 0) dn++; else up++; if (abs(d) >= 5 && abs(d) < 12) leap++; if (abs(d) == 12) oct++; }
          last = n;
        }
      }
    }
    const double N = notes;
    printf("%-12s %6.1f %5.0f%% %5.0f%% %5.0f%% %5.0f%% %5.0f%% %6.1f", compas::STYLES[st].name, N / cycles, 100 * rep / N, 100 * dn / N, 100 * up / N, 100 * leap / N, 100 * oct / N, height / N);
    if (compas::STYLES[st].flags & compas::F_ACID) printf("  acentos %.0f%% slides %.0f%%", 100 * acc / N, 100 * tie / N);
    printf("\n");
  }
}
