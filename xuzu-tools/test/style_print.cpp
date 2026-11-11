#include "xuzu/compas_engine.h"
#include <cstdio>
#include <cstdlib>
#include <string>
// one style, 2 cycles: rows A (accents), B (fills), C (chords), M (melody note at the semicorchea)
int main(int argc, char **argv) {
  static const char *N[12] = {"E","F","f","G","g","A","b","B","C","c","D","d"};   // lowercase = sharp/flat
  const int st = atoi(argv[1]); const int dens = argc > 2 ? atoi(argv[2]) : 55;
  compas::Engine e; e.style = st; e.density = dens; e.Restart(777);
  const int len = compas::STYLES[st].len, beat = compas::STYLES[st].beat;
  printf("== %s (len %d corcheas, %d per beat, dens %d)\n", compas::STYLES[st].name, len, beat, dens);
  for (int cyc = 0; cyc < 3; ++cyc) {
    std::string A, B, C, M, X, L;
    char chord[8] = "";
    for (int t = 0; t < len; ++t) {
      compas::Event ev = e.Tick();
      if (t == 0) e.ChordName(chord);
      for (int h = 0; h < 2; ++h) {
        const int k = h * 3;
        if ((t * 2 + h) % (beat * 2) == 0) { A += "|"; B += "|"; C += "|"; M += "|"; X += "|"; L += "|"; }
        A += (ev.fine_a >> k) & 1 ? (ev.remate ? "R" : "X") : ".";
        B += (ev.fine_b >> k) & 1 ? "x" : ((ev.fine_b >> (k + 1)) & 1 || (ev.fine_b >> (k + 2)) & 1) ? "," : ".";
        C += (ev.comp >> k) & 1 ? "c" : ".";
        char m = '.';
        for (int i = 0; i < ev.mel_n; ++i) if (ev.mel_at[i] / 3 == h && ((ev.mel_notes >> i) & 1)) { int v = ev.mel_semis[i]; m = N[((v % 12) + 12) % 12][0]; }
        M += m;
        char bl = '.'; for (int i = 0; i < ev.bass_n; ++i) if (ev.bass_at[i] / 3 == h) { int v = ev.bass_semis[i]; bl = N[((v % 12) + 12) % 12][0]; if (v >= 12) bl = bl - 'A' + 'a'; }
        L += bl;
        X += (ev.vmask[0] >> k) & 1 ? "p" : (ev.vmask[2] >> k) & 1 ? "k" : ".";
      }
    }
    printf(" ciclo %d acorde inicial %s\n  A %s\n  B %s\n  C %s\n  V %s\n  M %s\n  L %s\n", cyc, chord, A.c_str(), B.c_str(), C.c_str(), X.c_str(), M.c_str(), L.c_str());
  }
}
