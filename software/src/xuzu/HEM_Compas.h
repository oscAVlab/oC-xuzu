// Compas applet (xuzu rebuild of v4.16): a Compas of its own in half a screen.
// TR1 clock (x4 or x2 of the Pam)  TR2 reset   CV1 density   CV2 soltura
// Outputs: Trig+Mel (default), A+B, C+D, Acen+Rell, Raiz+Mel
// MIDI like the app: ch10 percussion, ch1 melody, ch2 chords, ch3 bass
#pragma once
#include "xuzu/compas_engine.h"

class CompasApplet : public HemisphereApplet {
public:
  enum Cur { STYLE, DENS, SOLT, OUTS, FAM, CLK, MIDI, NUEVO, CURSOR_LAST = NUEVO };
  enum Outs { O_TRIGMEL = 0, O_AB, O_CD, O_ACCFILL, O_ROOTMEL, NUM_OUTS };

  const char *applet_name() { return "Compas"; }

  void Start() {
    seed = 0xC0A5u ^ (OC::CORE::ticks << 3) ^ (uint32_t)hemisphere * 977u;
    engine.Restart(seed);
    engine.perfil = 0;
  }

  void Controller() {
    engine.style = style; engine.fam = fam;
    engine.density = constrain(dens + Proportion(DetentedIn(0), HEMISPHERE_MAX_INPUT_CV, 100), 0, 100);
    engine.soltura = constrain(solt + Proportion(DetentedIn(1), HEMISPHERE_MAX_INPUT_CV, 100), 0, 100);

    if (since < 1000000) since++;
    if (Clock(1)) { engine.Realign(); count = 0; }
    if (Clock(0)) {
      const int period = since > 16667 ? 2000 : since;
      if (since > 16667) { engine.Realign(); count = 0; }
      since = 0;
      const bool flam = engine.Def().flags & compas::F_CELLS;
      const bool x4 = clk == 0;
      if ((x4 && flam) || (!x4 && !flam)) Tick(period);
      else if (x4 && !flam) { if ((count++ & 1) == 0) Tick(period * 2); }
      else { Tick(period / 2); half = period / 2; }
    }
    if (half > 0 && --half == 0) Tick(last_tp);
    for (int i = 0; i < QN; ++i) {
      if (q_t[i] < 0 || --q_t[i] > 0) continue;
      q_t[i] = -1; Do(q_k[i], q_v[i], q_a[i]);
    }
    if (glide_left > 0) { --glide_left; if (MelOut()) Out(1, MelCV()); }
  }

  void View() {
    static const char *const ON[NUM_OUTS] = {"Trig+Mel", "A+B", "C+D", "Acen+Rell", "Raiz+Mel"};
    gfxPrint(1, 15, compas::STYLES[style].name);
    gfxPrint(1, 25, "Dens "); gfxPrint(dens);
    gfxPrint(1, 35, "Solt "); gfxPrint(solt);
    gfxPrint(1, 45, ON[outs]);
    switch (cursor) {
      case FAM: gfxPrint(1, 55, compas::FAMS[fam].name); break;
      case CLK: gfxPrint(1, 55, clk ? "x2 Pam" : "x4 Pam"); break;
      case MIDI: gfxPrint(1, 55, "MIDI "); gfxPrint(midi ? "Si" : "No"); break;
      case NUEVO: gfxPrint(1, 55, "Nuevo: gira >"); break;
      default: { char c[10]; engine.ChordName(c); gfxPrint(1, 55, c); gfxPrint(40, 55, engine.tick < 0 ? 0 : engine.CountAt(engine.Beat())); break; }
    }
    static const uint8_t Y[CURSOR_LAST + 1] = {15, 25, 35, 45, 55, 55, 55, 55};
    gfxCursor(1, Y[cursor] + 8, 62);
  }

  void OnEncoderMove(int dir) {
    if (!EditMode()) { MoveCursor(cursor, dir, CURSOR_LAST); return; }
    switch (cursor) {
      case STYLE: style = constrain(style + dir, 0, compas::NUM_STYLES - 1); break;
      case DENS: dens = constrain(dens + dir * 5, 0, 100); break;
      case SOLT: solt = constrain(solt + dir * 5, 0, 100); break;
      case OUTS: outs = constrain(outs + dir, 0, NUM_OUTS - 1); break;
      case FAM: fam = constrain(fam + dir, 0, compas::NUM_FAMS - 1); break;
      case CLK: clk = constrain(clk + dir, 0, 1); break;
      case MIDI: midi = constrain(midi + dir, 0, 1); break;
      case NUEVO: if (dir > 0) { seed = seed * 1664525u + 1013904223u + OC::CORE::ticks; engine.Restart(seed); } break;
    }
  }

  uint64_t OnDataRequest() {
    uint64_t d = 0;
    Pack(d, PackLocation{0, 4}, style & 15); Pack(d, PackLocation{4, 7}, dens); Pack(d, PackLocation{11, 7}, solt);
    Pack(d, PackLocation{18, 3}, outs); Pack(d, PackLocation{21, 3}, fam); Pack(d, PackLocation{24, 1}, clk);
    Pack(d, PackLocation{25, 1}, midi ? 0 : 1); Pack(d, PackLocation{26, 1}, style >> 4); Pack(d, PackLocation{32, 32}, seed);
    return d;
  }
  void OnDataReceive(uint64_t d) {
    style = constrain((int)(Unpack(d, PackLocation{0, 4}) | (Unpack(d, PackLocation{26, 1}) << 4)), 0, compas::NUM_STYLES - 1);
    dens = constrain((int)Unpack(d, PackLocation{4, 7}), 0, 100);
    solt = constrain((int)Unpack(d, PackLocation{11, 7}), 0, 100);
    outs = constrain((int)Unpack(d, PackLocation{18, 3}), 0, NUM_OUTS - 1);
    fam = constrain((int)Unpack(d, PackLocation{21, 3}), 0, compas::NUM_FAMS - 1);
    clk = Unpack(d, PackLocation{24, 1}); midi = !Unpack(d, PackLocation{25, 1});
    seed = (uint32_t)Unpack(d, PackLocation{32, 32});
    engine.style = style; engine.fam = fam;
    engine.Restart(seed);     // the preset brings back the same material
  }

protected:
  void SetHelp() {
    help[HELP_DIGITAL1] = "Clock";
    help[HELP_DIGITAL2] = "Reset";
    help[HELP_CV1] = "Densid";
    help[HELP_CV2] = "Soltura";
    help[HELP_OUT1] = outs == O_ROOTMEL ? "Raiz" : "Trig";
    help[HELP_OUT2] = (outs == O_ACCFILL) ? "Rell" : (outs == O_CD ? "Raiz" : "Melod");
    help[HELP_EXTRA1] = "Reloj: x4 o x2 del Pam";
    help[HELP_EXTRA2] = "MIDI: 10 perc 1 mel 2 3";
  }

private:
  compas::Engine engine;
  int cursor = 0;
  int style = 0, dens = 55, solt = 15, outs = O_TRIGMEL, fam = 0, clk = 0, midi = 1;
  uint32_t seed = 1;
  int since = 1000000, count = 0, half = 0, last_tp = 1000;
  int8_t mel_note = -1, bass_note = -1;
  int mel_semis = 12, glide_left = 0, glide_len = 1, glide_from = 0; bool slide_next = false;
  int8_t chord_on[4] = {-1, -1, -1, -1};
  enum { K_ACC = 1, K_FILL, K_MEL, K_OFF };
  static constexpr int QN = 24;
  int q_t[QN] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  uint8_t q_k[QN]; int16_t q_v[QN]; uint8_t q_a[QN];

  void Enq(int d, uint8_t k, int16_t v, uint8_t a) {
    if (d <= 0) { Do(k, v, a); return; }
    for (int i = 0; i < QN; ++i) if (q_t[i] < 0) { q_t[i] = d; q_k[i] = k; q_v[i] = v; q_a[i] = a; return; }
  }
  void Tick(int tp) {
    last_tp = tp;
    const compas::Event ev = engine.Tick();
    for (int k = 0; k < compas::FINE; ++k) {
      const int d = tp * k / compas::FINE;
      if (ev.fine_a & (1u << k)) Enq(d, K_ACC, 0, ev.remate ? 127 : ev.vel_a);
      if (ev.fine_b & (1u << k)) Enq(d, K_FILL, 0, ev.vel_b[k] ? ev.vel_b[k] : 80);
      for (int v = 0; v < compas::NUM_EXTRA_VOICES; ++v) if (ev.vmask[v] & (1u << k)) Enq(d, K_OFF, 39 + v, ev.vvel[v]);   // drum voices
    }
    for (int i = 0; i < ev.mel_n; ++i) {
      const bool note = (ev.mel_notes >> i) & 1, acc = (ev.mel_acc >> i) & 1, tie = (ev.mel_tie >> i) & 1;
      Enq(tp * ev.mel_at[i] / compas::FINE, K_MEL, (int16_t)((ev.mel_semis[i] & 0xFF) | (note ? 0x100 : 0) | (tie ? 0x200 : 0)), acc ? 122 : note ? 95 : 70);
    }
    if (midi && (ev.fine_a & 1u)) {   // bass: the root on every accent, chords with the rhythm
      if (bass_note >= 0) usbMIDI.sendNoteOff(bass_note, 0, 3);
      bass_note = (int8_t)(40 + engine.RootSemis() % 12); usbMIDI.sendNoteOn(bass_note, 100, 3);
    }
    if (midi && ev.comp) Chord(ev.comp_vel);
    if (midi && ev.remate) { usbMIDI.sendNoteOn(49, 110, 10); usbMIDI.sendNoteOff(49, 0, 10); }
  }
  void Chord(uint8_t vel) {
    for (int i = 0; i < 4; ++i) if (chord_on[i] >= 0) { usbMIDI.sendNoteOff(chord_on[i], 0, 2); chord_on[i] = -1; }
    int8_t iv[4]; const int n = engine.ChordTones(iv);
    const int root = 52 + ((engine.RootSemis() % 12) + 12) % 12;
    for (int i = 0; i < n; ++i) { chord_on[i] = (int8_t)(root + iv[i]); usbMIDI.sendNoteOn(chord_on[i], vel, 2); }
  }
  bool MelOut() const { return outs == O_TRIGMEL || outs == O_AB || outs == O_ROOTMEL; }
  int MelCV() const { const int to = mel_semis * 128; return glide_left > 0 ? to + (glide_from - to) * glide_left / glide_len : to; }
  void Drum(int note, int vel) { if (midi) { usbMIDI.sendNoteOn(note, vel, 10); usbMIDI.sendNoteOff(note, 0, 10); } }
  void Do(uint8_t k, int16_t v, uint8_t a) {
    switch (k) {
      case K_ACC:
        Drum(36, a);
        if (outs == O_TRIGMEL || outs == O_AB || outs == O_ACCFILL) ClockOut(0);
        break;
      case K_FILL:
        Drum(38, a);
        if (outs == O_TRIGMEL || outs == O_CD) ClockOut(0);
        if (outs == O_ACCFILL) ClockOut(1);
        break;
      case K_MEL: {
        const int semis = (int8_t)(v & 0xFF);
        if (v & 0x100) {      // a real note: the one after a tied note slides (acid)
          if (slide_next) { glide_from = MelCV(); glide_len = glide_left = constrain(last_tp / 4, 17, 1700); }
          else glide_left = 0;
          slide_next = v & 0x200;
        }
        mel_semis = semis;
        if (MelOut()) Out(1, MelCV());
        if (midi) { if (mel_note >= 0) usbMIDI.sendNoteOff(mel_note, 0, 1); mel_note = (int8_t)constrain(64 + semis, 0, 127); usbMIDI.sendNoteOn(mel_note, a, 1); }
        break;
      }
      case K_OFF: Drum(v, a); break;
    }
    if (outs == O_CD) Out(1, engine.RootSemis() * 128);
    if (outs == O_ROOTMEL) Out(0, engine.RootSemis() * 128);
  }
};
