// Just Friends applets (osc[AV]lab v4.25): the o_C plays a Mannequins Just Friends over I2C (ii), no CV cables.
//  JF Synth : synthesis mode, 6 voices. Note / Chord / Arp from CV1 (quantized), or follows the Compas applet.
//  JF Geode : geode mode, rhythms per voice (divisions x repeats) in time with the o_C clock.
//  JF Ctrl  : shape mode. TR1/TR2 strike outputs, CV1 = RUN, CV2 = transpose TIME, INTONE retuning, 432 Hz, quantize.
// Messages go through xuzu/jf_ii.h (queued in the ISR, sent from the main loop). I2C exists only on the o_C T4.1.
#pragma once
#include "xuzu/hem_list.h"
#include "xuzu/harm_share.h"
#include "xuzu/jf_ii.h"

class JFBase : public XuzuListApplet {
protected:
  int jfw = 0, flash = 0;
  void Firma(int x, int y) {                     // ž, 4x6 px
    static const uint8_t f[6] = {0b1010, 0b0100, 0b1111, 0b0010, 0b0100, 0b1111};
    for (int r = 0; r < 6; ++r) for (int c = 0; c < 4; ++c) if (f[r] & (8 >> c)) gfxPixel(x + c, y + r);
  }
  void DrawJF() {
    DrawList(false);
    Firma(58, 16);
    if (flash) gfxInvert(57, 15, 6, 8);
  }
  bool TopStatus(const char *extra) {
    gfxPrint(1, 15, jf::Status(jfw));
    if (extra) gfxPrint(34, 15, extra);
    return true;
  }
  void Hit() { flash = 1500; }
  void Tick() { if (flash) flash--; }
  static int Lvl(int tenths) { return constrain((int)jf::Level(tenths), 0, 16383); }
};

// ---------------------------------------------------------------- JF Synth
class JFSynth : public JFBase {
public:
  enum Prm { P_MODO, P_JF, P_ESC, P_ACORDE, P_VOCES, P_ARP, P_OCT, P_TRANSP, P_NIVEL, P_CV2, P_PROBAR, NUM_P };
  enum Modo { M_NOTA, M_ACORDE, M_ARP, M_COMPAS, NUM_M };
  static constexpr int NUM_CH = 9;

  const char *applet_name() { return "JF Synth"; }
  void Start() {}
  void Controller() {
    Tick();
    jf::WantMode(jfw, 1);
    if (probe) { probe = false; jf::Note(jfw, jf::Semis(12 * oct + transp), Lvl(nivel * 10)); Hit(); }
    if (Clock(1)) {
      if (modo == M_ARP) { arp_i = 0; arp_up = true; }
      else if (modo == M_COMPAS) Mel();
      else { jf::Vox(jfw, 0, 0, 0); }            // all voices off (for JF in sustain)
    }
    if (Clock(0)) { StartADCLag(0); pending = true; }
    if (pending && EndOfADCLag(0)) { pending = false; Fire(); }
  }
  void View() { DrawJF(); }
  void OnEncoderMove(int dir) { ListEncoder(dir); }
  uint64_t OnDataRequest() {
    uint64_t d = 0;
    Pack(d, PackLocation{0, 2}, modo); Pack(d, PackLocation{2, 1}, jfw); Pack(d, PackLocation{3, 4}, acorde);
    Pack(d, PackLocation{7, 3}, voces - 1); Pack(d, PackLocation{10, 2}, arp); Pack(d, PackLocation{12, 3}, oct + 3);
    Pack(d, PackLocation{15, 5}, transp + 12); Pack(d, PackLocation{20, 4}, nivel); Pack(d, PackLocation{24, 1}, cv2);
    return d;
  }
  void OnDataReceive(uint64_t d) {
    modo = Unpack(d, PackLocation{0, 2}); jfw = Unpack(d, PackLocation{2, 1});
    acorde = constrain((int)Unpack(d, PackLocation{3, 4}), 0, NUM_CH - 1); voces = Unpack(d, PackLocation{7, 3}) + 1;
    voces = constrain(voces, 1, 6); arp = Unpack(d, PackLocation{10, 2}); oct = (int)Unpack(d, PackLocation{12, 3}) - 3;
    oct = constrain(oct, -3, 3); transp = constrain((int)Unpack(d, PackLocation{15, 5}) - 12, -12, 12);
    nivel = constrain((int)Unpack(d, PackLocation{20, 4}), 1, 10); cv2 = Unpack(d, PackLocation{24, 1});
  }

protected:
  void SetHelp() {
    help[HELP_DIGITAL1] = "Tocar";
    help[HELP_DIGITAL2] = modo == M_ARP ? "Reset" : modo == M_COMPAS ? "Melod" : "Soltar";
    help[HELP_CV1] = "Tono";
    help[HELP_CV2] = cv2 ? "Nivel" : "";
    help[HELP_OUT1] = "Tono";
    help[HELP_OUT2] = "Trig";
    help[HELP_EXTRA1] = "Just Friends por I2C";
    help[HELP_EXTRA2] = "modo synthesis, 6 voces";
  }
  bool Top() {
    static const char *const M[NUM_M] = {"Nota", "Acor", "Arp", "Comp"};
    return TopStatus(modo == M_COMPAS && !CompasOn() ? "sinC" : M[modo]);
  }
  int NumParams() { return NUM_P; }
  const char *Name(int i) {
    static const char *const N[NUM_P] = {"Modo", "JF", "Escal", "Acord", "Voces", "Arp", "Octav", "Trans", "Nivel", "CV2", "Probr"};
    return N[i];
  }
  void Value(int i, char *b, bool full) {
    const int m = full ? 9 : 5;
    switch (i) {
      case P_MODO: { static const char *const M[NUM_M] = {"Nota", "Acorde", "Arpegio", "Compas"}; xuzu::Copy(b, M[modo], m); break; }
      case P_JF: xuzu::Itoa(jfw + 1, b); break;
      case P_ESC: xuzu::Copy(b, OC::scale_names_short[GetScale(0)], m); break;
      case P_ACORDE: xuzu::Copy(b, CH_NAME[acorde], m); break;
      case P_VOCES: xuzu::Itoa(voces, b); break;
      case P_ARP: { static const char *const A[4] = {"Sube", "Baja", "S-B", "Azar"}; xuzu::Copy(b, A[arp], m); break; }
      case P_OCT: xuzu::Signed(oct, b); break;
      case P_TRANSP: xuzu::Signed(transp, b); break;
      case P_NIVEL: xuzu::Itoa(nivel, b); if (full) { const int l = strlen(b); xuzu::Copy(b + l, " V", 9 - l); } break;
      case P_CV2: xuzu::Copy(b, cv2 ? "Nivel" : "Off", m); break;
      case P_PROBAR: xuzu::Copy(b, full ? "gira=nota" : "gira", m); break;
    }
  }
  void Change(int i, int dir) {
    switch (i) {
      case P_MODO: modo = constrain(modo + dir, 0, NUM_M - 1); break;
      case P_JF: jfw = dir > 0; break;
      case P_ESC: NudgeScale(0, dir); break;
      case P_ACORDE: acorde = constrain(acorde + dir, 0, NUM_CH - 1); break;
      case P_VOCES: voces = constrain(voces + dir, 1, 6); break;
      case P_ARP: arp = constrain(arp + dir, 0, 3); break;
      case P_OCT: oct = constrain(oct + dir, -3, 3); break;
      case P_TRANSP: transp = constrain(transp + dir, -12, 12); break;
      case P_NIVEL: nivel = constrain(nivel + dir, 1, 10); break;
      case P_CV2: cv2 = dir > 0; break;
      case P_PROBAR: probe = true; break;
    }
  }

private:
  static constexpr int8_t CH[NUM_CH][6] = {
    {0, 4, 7, 12, 16, 19}, {0, 3, 7, 12, 15, 19}, {0, 4, 7, 11, 14, 19}, {0, 3, 7, 10, 14, 17}, {0, 4, 7, 10, 14, 19},
    {0, 2, 7, 12, 14, 19}, {0, 5, 7, 12, 17, 19}, {0, 7, 12, 19, 24, 31}, {0, 3, 7, 10, 14, 21}};
  static constexpr const char *CH_NAME[NUM_CH] = {"Mayor", "menor", "Maj7", "m7", "7", "sus2", "sus4", "5tas", "m9"};
  int modo = M_NOTA, acorde = 0, voces = 4, arp = 0, oct = 0, transp = 0, nivel = 5, cv2 = 1;
  int arp_i = 0; bool arp_up = true, pending = false, probe = false;
  uint32_t rnd = 12345;

  bool CompasOn() const { return xuzu::Recent(xuzu::compas_seen_tick, OC::CORE::ticks); }
  int Velo() {
    int v = nivel * 10 + (cv2 ? In(1) * 10 / 1536 : 0);    // tenths of a volt
    return Lvl(constrain(v, 0, 100));
  }
  void Played(int semis) { Out(0, semis * 128); ClockOut(1); Hit(); }
  void Fire() {
    const int vel = Velo();
    if (modo == M_COMPAS) {
      if (!CompasOn()) return;
      const xuzu::Harm &h = xuzu::compas_harm;
      const int base = h.root + 12 * oct + transp;
      for (int i = 0; i < h.n && i < 4; ++i) if (h.tones[i] >= 0) jf::Vox(jfw, i + 1, jf::Semis(base + h.tones[i]), vel);
      Played(base);
      return;
    }
    const int root = Quantize(0, In(0)) / 128 + 12 * oct + transp;
    if (modo == M_NOTA) { jf::Note(jfw, jf::Semis(root), vel); Played(root); return; }
    if (modo == M_ACORDE) {
      for (int i = 0; i < voces; ++i) jf::Vox(jfw, i + 1, jf::Semis(root + CH[acorde][i]), vel);
      Played(root);
      return;
    }
    // arpeggio over the chord tones
    int idx;
    switch (arp) {
      case 0: idx = arp_i % voces; arp_i = (arp_i + 1) % voces; break;
      case 1: idx = voces - 1 - arp_i % voces; arp_i = (arp_i + 1) % voces; break;
      case 2:
        if (voces < 2) { idx = 0; break; }
        idx = arp_i;
        if (arp_up) { if (++arp_i >= voces - 1) arp_up = false; } else { if (--arp_i <= 0) arp_up = true; }
        break;
      default: rnd = rnd * 1103515245u + 12345u; idx = (rnd >> 16) % voces; break;
    }
    const int s = root + CH[acorde][constrain(idx, 0, 5)];
    jf::Note(jfw, jf::Semis(s), vel);
    Played(s);
  }
  void Mel() {
    if (!CompasOn()) return;
    const int s = xuzu::compas_harm.mel + 12 * oct;
    jf::Vox(jfw, 6, jf::Semis(s), Velo());
    Played(s);
  }
};

// ---------------------------------------------------------------- JF Geode
class JFGeode : public JFBase {
public:
  enum Prm { P_JF, P_RELOJ, P_VOZ, P_DIVS, P_REP, P_CADA, P_CV1, P_CV2, NUM_P };
  const char *applet_name() { return "JF Geode"; }
  void Start() {}
  void Controller() {
    Tick();
    jf::WantMode(jfw, 1);
    const uint32_t now = OC::CORE::ticks;
    if (Clock(0)) {
      if (tick_at[jfw] != now) { tick_at[jfw] = now; jf::Tick(jfw, TPM[reloj]); }   // one tick per clock even with 2 Geode applets
      ClockOut(0);
      if (cada && ++count >= cada) { count = 0; Fire(); }
    }
    if (Clock(1)) Fire();
  }
  void View() { DrawJF(); }
  void OnEncoderMove(int dir) { ListEncoder(dir); }
  uint64_t OnDataRequest() {
    uint64_t d = 0;
    Pack(d, PackLocation{0, 1}, jfw); Pack(d, PackLocation{1, 3}, reloj); Pack(d, PackLocation{4, 3}, voz);
    Pack(d, PackLocation{7, 6}, divs); Pack(d, PackLocation{13, 6}, rep); Pack(d, PackLocation{19, 5}, cada);
    Pack(d, PackLocation{24, 1}, cv1); Pack(d, PackLocation{25, 1}, cv2);
    return d;
  }
  void OnDataReceive(uint64_t d) {
    jfw = Unpack(d, PackLocation{0, 1}); reloj = constrain((int)Unpack(d, PackLocation{1, 3}), 0, 6);
    voz = constrain((int)Unpack(d, PackLocation{4, 3}), 0, 6); divs = constrain((int)Unpack(d, PackLocation{7, 6}), 1, 32);
    rep = constrain((int)Unpack(d, PackLocation{13, 6}), 1, 32); cada = constrain((int)Unpack(d, PackLocation{19, 5}), 0, 16);
    cv1 = Unpack(d, PackLocation{24, 1}); cv2 = Unpack(d, PackLocation{25, 1});
  }

protected:
  void SetHelp() {
    help[HELP_DIGITAL1] = "Reloj";
    help[HELP_DIGITAL2] = "Disp";
    help[HELP_CV1] = cv1 ? "Divs" : "";
    help[HELP_CV2] = cv2 ? "Repet" : "";
    help[HELP_OUT1] = "Reloj";
    help[HELP_OUT2] = "Disp";
    help[HELP_EXTRA1] = "Just Friends por I2C";
    help[HELP_EXTRA2] = "modo geode";
  }
  bool Top() { return TopStatus(voz ? "Voz" : "Auto"); }
  int NumParams() { return NUM_P; }
  const char *Name(int i) {
    static const char *const N[NUM_P] = {"JF", "Reloj", "Voz", "Divs", "Repet", "Cada", "CV1", "CV2"};
    return N[i];
  }
  void Value(int i, char *b, bool full) {
    const int m = full ? 9 : 5;
    switch (i) {
      case P_JF: xuzu::Itoa(jfw + 1, b); break;
      case P_RELOJ: xuzu::Itoa(TPM[reloj], b); if (full) { const int l = strlen(b); xuzu::Copy(b + l, "/compas", 9 - l); } break;
      case P_VOZ: if (voz) xuzu::Itoa(voz, b); else xuzu::Copy(b, "Auto", m); break;
      case P_DIVS: xuzu::Itoa(divs, b); break;
      case P_REP: xuzu::Itoa(rep, b); break;
      case P_CADA:
        if (!cada) { xuzu::Copy(b, full ? "solo TR2" : "TR2", m); break; }
        xuzu::Itoa(cada, b); if (full) { const int l = strlen(b); xuzu::Copy(b + l, " pulsos", 9 - l); }
        break;
      case P_CV1: xuzu::Copy(b, cv1 ? "Divs" : "Off", m); break;
      case P_CV2: xuzu::Copy(b, cv2 ? "Repet" : "Off", m); break;
    }
  }
  void Change(int i, int dir) {
    switch (i) {
      case P_JF: jfw = dir > 0; break;
      case P_RELOJ: reloj = constrain(reloj + dir, 0, 6); break;
      case P_VOZ: voz = constrain(voz + dir, 0, 6); break;
      case P_DIVS: divs = constrain(divs + dir, 1, 32); break;
      case P_REP: rep = constrain(rep + dir, 1, 32); break;
      case P_CADA: cada = constrain(cada + dir, 0, 16); break;
      case P_CV1: cv1 = dir > 0; break;
      case P_CV2: cv2 = dir > 0; break;
    }
  }

private:
  static constexpr int8_t TPM[7] = {4, 8, 12, 16, 24, 32, 48};     // ticks per bar: 16 = TR1 in 16ths (Pam x4)
  int reloj = 3, voz = 0, divs = 4, rep = 4, cada = 0, cv1 = 1, cv2 = 1, count = 0;
  static inline uint32_t tick_at[2] = {0xFFFFFFFFu, 0xFFFFFFFFu};

  void Fire() {
    const int d = constrain(divs + (cv1 ? xuzu::Bi(In(0), 16) : 0), 1, 64);
    const int r = constrain(rep + (cv2 ? xuzu::Bi(In(1), 16) : 0), 1, 64);
    if (voz) jf::Vox(jfw, voz, d, r); else jf::Note(jfw, d, r);
    ClockOut(1);
    Hit();
  }
};

// ---------------------------------------------------------------- JF Ctrl
class JFCtrl : public JFBase {
public:
  enum Prm { P_JF, P_SAL1, P_SAL2, P_NIVEL, P_RUN, P_CV2, P_AFIN, P_GOD, P_CUANT, NUM_P };
  static constexpr int NUM_AF = 5;
  const char *applet_name() { return "JF Ctrl"; }
  void Start() {}
  void Controller() {
    Tick();
    jf::WantMode(jfw, 0);
    for (int ch = 0; ch < 2; ++ch) {
      const int sal = ch ? sal2 : sal1;
      if (Clock(ch)) { jf::VTrigger(jfw, sal, Lvl(nivel * 10)); Hit(); gate[ch] = true; }
      else if (gate[ch] && !Gate(ch)) { jf::Trigger(jfw, sal, 0); gate[ch] = false; }
    }
    if (++div >= 64) {                          // ~4 ms: RUN / transpose follow the CVs without flooding the bus
      div = 0;
      if (run && abs(In(0) - last_run) > 12) { last_run = In(0); jf::Run(jfw, jf::FromHS(last_run)); }
      if (cv2 && abs(In(1) - last_sh) > 12) { last_sh = In(1); jf::Shift(jfw, jf::FromHS(last_sh)); }
      if (++keep >= 512) { keep = 0; jf::RunMode(jfw, run); }      // ~2 s, in case the JF was power-cycled
    }
  }
  void View() { DrawJF(); }
  void OnEncoderMove(int dir) { ListEncoder(dir); }
  uint64_t OnDataRequest() {
    uint64_t d = 0;
    Pack(d, PackLocation{0, 1}, jfw); Pack(d, PackLocation{1, 3}, sal1); Pack(d, PackLocation{4, 3}, sal2);
    Pack(d, PackLocation{7, 4}, nivel); Pack(d, PackLocation{11, 1}, run); Pack(d, PackLocation{12, 1}, cv2);
    Pack(d, PackLocation{13, 3}, afin); Pack(d, PackLocation{16, 1}, god); Pack(d, PackLocation{17, 5}, cuant);
    return d;
  }
  void OnDataReceive(uint64_t d) {
    jfw = Unpack(d, PackLocation{0, 1}); sal1 = constrain((int)Unpack(d, PackLocation{1, 3}), 0, 6);
    sal2 = constrain((int)Unpack(d, PackLocation{4, 3}), 0, 6); nivel = constrain((int)Unpack(d, PackLocation{7, 4}), 1, 10);
    run = Unpack(d, PackLocation{11, 1}); cv2 = Unpack(d, PackLocation{12, 1});
    afin = constrain((int)Unpack(d, PackLocation{13, 3}), 0, NUM_AF - 1); god = Unpack(d, PackLocation{16, 1});
    cuant = constrain((int)Unpack(d, PackLocation{17, 5}), 0, 16);
    SendAll();
  }

protected:
  void SetHelp() {
    help[HELP_DIGITAL1] = "Sal 1";
    help[HELP_DIGITAL2] = "Sal 2";
    help[HELP_CV1] = run ? "RUN" : "";
    help[HELP_CV2] = cv2 ? "TIME" : "";
    help[HELP_OUT1] = "";
    help[HELP_OUT2] = "";
    help[HELP_EXTRA1] = "Just Friends por I2C";
    help[HELP_EXTRA2] = "modo shape";
  }
  bool Top() { return TopStatus(run ? "RUN" : nullptr); }
  int NumParams() { return NUM_P; }
  const char *Name(int i) {
    static const char *const N[NUM_P] = {"JF", "Sal1", "Sal2", "Nivel", "Run", "CV2", "Afin", "432", "Cuant"};
    return N[i];
  }
  void Value(int i, char *b, bool full) {
    const int m = full ? 9 : 5;
    switch (i) {
      case P_JF: xuzu::Itoa(jfw + 1, b); break;
      case P_SAL1: case P_SAL2: { const int s = i == P_SAL1 ? sal1 : sal2; if (s) xuzu::Itoa(s, b); else xuzu::Copy(b, "Todas", m); break; }
      case P_NIVEL: xuzu::Itoa(nivel, b); if (full) { const int l = strlen(b); xuzu::Copy(b + l, " V", 9 - l); } break;
      case P_RUN: xuzu::Copy(b, run ? (full ? "CV1 = RUN" : "CV1") : "Off", m); break;
      case P_CV2: xuzu::Copy(b, cv2 ? (full ? "Transp." : "Trans") : "Off", m); break;
      case P_AFIN: { static const char *const A[NUM_AF] = {"Normal", "Armonicos", "Impares", "Acorde", "Quintas"}; xuzu::Copy(b, A[afin], m); break; }
      case P_GOD: xuzu::Copy(b, god ? "Si" : "No", m); break;
      case P_CUANT: if (cuant) { xuzu::Copy(b, "1/", m); xuzu::Itoa(cuant, b + 2); } else xuzu::Copy(b, "Off", m); break;
    }
  }
  void Change(int i, int dir) {
    switch (i) {
      case P_JF: jfw = dir > 0; SendAll(); break;
      case P_SAL1: sal1 = constrain(sal1 + dir, 0, 6); break;
      case P_SAL2: sal2 = constrain(sal2 + dir, 0, 6); break;
      case P_NIVEL: nivel = constrain(nivel + dir, 1, 10); break;
      case P_RUN: run = dir > 0; jf::RunMode(jfw, run); last_run = -99999; break;
      case P_CV2: cv2 = dir > 0; if (!cv2) jf::Shift(jfw, 0); last_sh = -99999; break;
      case P_AFIN: afin = constrain(afin + dir, 0, NUM_AF - 1); SendTune(); break;
      case P_GOD: god = dir > 0; jf::God(jfw, god); break;
      case P_CUANT: cuant = constrain(cuant + dir, 0, 16); jf::Quant(jfw, cuant); break;
    }
  }

private:
  int sal1 = 1, sal2 = 2, nivel = 5, run = 0, cv2 = 0, afin = 0, god = 0, cuant = 0;
  int div = 0, keep = 0, last_run = -99999, last_sh = -99999;
  bool gate[2] = {false, false};

  void SendTune() {
    static const int8_t R[NUM_AF - 1][6][2] = {
      {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}}, {{1, 1}, {3, 1}, {5, 1}, {7, 1}, {9, 1}, {11, 1}},
      {{1, 1}, {5, 4}, {3, 2}, {2, 1}, {5, 2}, {3, 1}}, {{1, 1}, {3, 2}, {2, 1}, {3, 1}, {4, 1}, {6, 1}}};
    if (!afin) { jf::Tune(jfw, 0, 0, 0); return; }        // back to the INTONE knob
    for (int c = 0; c < 6; ++c) jf::Tune(jfw, c + 1, R[afin - 1][c][0], R[afin - 1][c][1]);
  }
  void SendAll() { jf::RunMode(jfw, run); SendTune(); jf::God(jfw, god); jf::Quant(jfw, cuant); }
};
