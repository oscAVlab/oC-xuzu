#include "xuzu/compas_engine.h"
#include "xuzu/euclid4_engine.h"
#include "xuzu/brains_engine.h"
#include <cstdio>
#include <string>
euclid4::Engine euclid_engine;
brains::Engine brains_engine;
static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FALLA: " __VA_ARGS__); printf("\n"); } } while (0)

static std::string Ev(const compas::Event &e, const compas::Engine &c) {
  char b[128]; snprintf(b, sizeof b, "%02x%02x%02x%d|", e.fine_a, e.fine_b, e.comp, e.mel_n);
  std::string s = b; for (int i = 0; i < e.mel_n; ++i) { snprintf(b, sizeof b, "%d@%d,", e.mel_semis[i], e.mel_at[i]); s += b; }
  snprintf(b, sizeof b, "r%d;", c.RootSemis()); return s + b;
}
static std::string Run(compas::Engine &c, int n) { std::string s; for (int i = 0; i < n; ++i) { compas::Event e = c.Tick(); s += Ev(e, c); } return s; }

int main() {
  // 1. Compas: the same seed always gives the same music (what the applet stores in a preset)
  for (int st = 0; st < compas::NUM_STYLES; ++st) for (int pf = 0; pf < 2; ++pf) {
    compas::Engine a, b; a.style = b.style = st; a.perfil = b.perfil = pf;
    a.Restart(1234 + st); b.Restart(1234 + st);
    CHECK(Run(a, 400) == Run(b, 400), "estilo %d perfil %d no es determinista con la misma semilla", st, pf);
    compas::Engine c; c.style = st; c.perfil = pf; c.Restart(999 + st);
    a.Restart(1234 + st);
    CHECK(Run(a, 200) != Run(c, 200), "estilo %d: semillas distintas dan lo mismo", st);
  }
  // 2. Compas: a saved material (app preset) brings back exactly the same continuation
  for (int st = 0; st < compas::NUM_STYLES; ++st) {
    compas::Engine a; a.style = st; a.Restart(77 + st); Run(a, 37);
    // stop at a cycle start so both engines resume from the same place
    compas::Engine::Material m; a.GetMaterial(m);
    compas::Engine b; b.style = st; b.Restart(5); b.SetMaterial(m);
    compas::Engine a2; a2.style = st; a2.Restart(5); a2.SetMaterial(m);
    CHECK(Run(b, 300) == Run(a2, 300), "estilo %d: el material guardado no se repite igual", st);
  }
  // 3. Compas: frozen rhythm + melody repeats with the loop length
  for (int st = 0; st < compas::NUM_STYLES; ++st) {
    compas::Engine a; a.style = st; a.Restart(31 + st); a.loop_r = 4; a.loop_m = 4; a.loop_h = 0; a.soltura = 0;
    Run(a, 4 * a.LoopTicks(4));
    a.lock = compas::L_TODO;
    const int L = a.LoopTicks(4);
    Run(a, 2 * L);
    std::string x = Run(a, L), y = Run(a, L);
    CHECK(x == y, "estilo %d: congelado no repite cada %d corcheas", st, L);
  }
  // 4. Compas: every style / profile / density runs without going out of bounds (ASan/UBSan)
  for (int st = 0; st < compas::NUM_STYLES; ++st) for (int pf = 0; pf < 2; ++pf) for (int d = 0; d <= 100; d += 25)
    for (int f = 0; f < compas::NUM_FAMS; ++f) {
      compas::Engine a; a.style = st; a.perfil = pf; a.density = d; a.fam = f; a.notas = d; a.adorno = 100 - d; a.Restart(d * 7 + f);
      for (int i = 0; i < 300; ++i) { a.Tick(); if (i % 50 == 0) a.Realign(); if (i == 150) a.Reroll(); }
    }
  // 5. Euclid x4: a track with steps/hits plays exactly the Euclidean pattern, one hit per 16th
  {
    euclid4::Engine &e = euclid_engine;
    for (int k = 0; k < 4; ++k) { e.t[k].prob = 100; e.t[k].azar = 0; e.t[k].burst = 0; e.t[k].ratio = 4; }
    e.t[0].len = 16; e.t[0].steps = 16; e.t[0].hits = 5; e.t[0].rot = 0;
    e.Reset();
    uint8_t want[16]; euclid4::Euclid(5, 16, want);
    std::string got, exp;
    for (int s = 0; s < 32; ++s) {
      e.Clock16(2000); bool h = false;
      for (int t = 0; t < 2000; ++t) { e.Update(); h |= e.hit[0]; e.EndTick(); }
      got += h ? '1' : '0'; exp += want[s % 16] ? '1' : '0';
    }
    CHECK(got == exp, "Euclid T1: %s esperaba %s", got.c_str(), exp.c_str());
  }
  // 5b. ratios: x2 gives two hits per 16th on a full pattern, /2 one every two
  {
    euclid4::Engine &e = euclid_engine;
    for (int ri : {3, 5}) {
      e.t[1].len = 16; e.t[1].steps = 16; e.t[1].hits = 16; e.t[1].ratio = ri; e.Reset();
      int n = 0;
      for (int s = 0; s < 16; ++s) { e.Clock16(2000); for (int t = 0; t < 2000; ++t) { e.Update(); n += e.hit[1]; e.EndTick(); } }
      CHECK(n == (ri == 3 ? 32 : 8), "ratio %s: %d golpes en 16 semicorcheas", euclid4::RATIO_NAMES[ri], n);
    }
    // a burst of 3 on every hit: 3 hits per step
    e.t[2].len = 4; e.t[2].steps = 4; e.t[2].hits = 4; e.t[2].ratio = 4; e.t[2].burst = 3; e.t[2].bhits = 4; e.t[2].bmode = 0; e.Reset();
    int n = 0;
    for (int s = 0; s < 8; ++s) { e.Clock16(2000); for (int t = 0; t < 2000; ++t) { e.Update(); n += e.hit[2]; e.EndTick(); } }
    CHECK(n == 24, "rafaga x3: %d golpes, esperaba 24", n);
  }
  // 6. Brains Seq: runs for a long time with loop and macro moves without going out of bounds
  {
    brains::Engine &b = brains_engine; b.Init();
    b.s.macro_amt[brains::MD_SLOTS] = 100; b.s.macro_amt[brains::MD_HITS] = -50; b.s.transition = 5;
    int trigs = 0;
    for (int s = 0; s < 64 * 20; ++s) {
      b.SetMacro((s * 37) % 7680); b.SetLoopGate((s / 128) % 2); b.SetTransition((s * 13) % 7680);
      b.Clock(2000);
      for (int t = 0; t < 2000; ++t) { b.Update(); trigs += b.trig; }
      CHECK(b.out_model >= 0 && b.out_model <= 7680 && b.ActiveSlots() >= 1 && b.ActiveSlots() <= 4, "Brains fuera de rango");
    }
    CHECK(trigs > 100, "Brains casi no dispara (%d)", trigs);
    printf("Brains: %d disparos en 80 compases\n", trigs);
  }
  printf(fails ? "%d FALLAS\n" : "todo bien\n", fails);
  return fails != 0;
}
