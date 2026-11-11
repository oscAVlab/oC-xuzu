// Brains applet (xuzu v4.18): plays and edits the slots of the Brains Seq app.
// TR1 clock, TR2 reset.  CV1 macro, CV2 loop (gate) or C/D transition.  Output 1 trigger to Brains, output 2 MODEL.
// What you edit here is the app's setup: it is kept with the app and its presets.
#pragma once
#include "xuzu/brains_engine.h"
#include "xuzu/hem_list.h"

class Brains : public XuzuListApplet {
public:
  enum Prm { P_PRESET, P_SLOT, P_SRC, P_MODEL, P_OCT, P_MACRO, P_MHITS, P_MPROB, P_MAZAR, P_MROT, P_MDENS, P_MSOLT, P_MSLOTS,
             P_LOOPLEN, P_FIXED, P_PRIO, P_TRANS, P_DELAY, P_CSTYLE, P_CDENS, P_CSOLT, P_CV2, P_CLK, NUM_P };

  const char *applet_name() { return "Brains"; }
  void Start() {}
  void Controller() {
    brains::Engine &e = brains_engine;
    const uint32_t now = OC::CORE::ticks;
    const bool owner = (last_owner_tick != now);
    if (owner) {
      last_owner_tick = now;
      xuzu::brains_seen_tick = now;
      if (!xuzu::Recent(xuzu::eucl_seen_tick, now)) euclid_engine.ClearHM();   // no Eucl x4 applet adding CV
      e.SetMacro(In(0));
      if (cv2 == 0) { e.SetLoopGate(In(1) > HEMISPHERE_3V_CV / 2); e.SetTransition(0); }
      else { e.SetLoopGate(false); e.SetTransition(In(1)); }
      if (since < 1000000) since++;
      if (Clock(1)) e.Reset();
      if (Clock(0)) {
        if (since > 16667) { e.Reset(); e.Clock(last); } else { e.Clock(since); last = since; }
        since = 0;
      }
      e.Update();
    }
    Out(1, e.out_model);
    if (e.trig) { ClockOut(0); flash = 600; }
    if (flash) flash--;
  }
  void View() { DrawList(flash > 0); }
  void OnEncoderMove(int dir) { ListEncoder(dir); }
  uint64_t OnDataRequest() {
    uint64_t d = 0;
    Pack(d, PackLocation{0, 5}, preset); Pack(d, PackLocation{5, 2}, slot); Pack(d, PackLocation{7, 1}, cv2);
    return d;
  }
  void OnDataReceive(uint64_t d) {
    preset = constrain((int)Unpack(d, PackLocation{0, 5}), 0, 16); slot = Unpack(d, PackLocation{5, 2}); cv2 = Unpack(d, PackLocation{7, 1});
    if (preset > 0) xuzu::BrainsLoadPreset(preset - 1);     // the Hemisphere preset brings its Brains setup
  }

protected:
  void SetHelp() {
    help[HELP_DIGITAL1] = "Clock";
    help[HELP_DIGITAL2] = "Reset";
    help[HELP_CV1] = "Macro";
    help[HELP_CV2] = cv2 ? "Trans" : "LOOP";
    help[HELP_OUT1] = "Trig";
    help[HELP_OUT2] = "Model";
    help[HELP_EXTRA1] = "Slots: app Brains Seq";
    help[HELP_EXTRA2] = "";
  }

  // top row: the 4 slots with their model, the one playing lit
  bool Top() {
    brains::Engine &e = brains_engine;
    for (int i = 0; i < 4; ++i) {
      const int x = 1 + i * 14;
      if (i >= e.ActiveSlots()) { gfxPrint(x, 15, "-"); continue; }
      gfxPrint(x, 15, e.s.slot[i].model + 1);
      if (e.last_slot == i && flash) gfxInvert(x - 1, 14, 8, 9);
      if (i == slot) gfxLine(x, 23, x + 5, 23);
    }
    if (e.looping) gfxPrint(46, 15, "L");
    return true;
  }
  int NumParams() { return NUM_P; }
  const char *Name(int i) {
    static const char *const N[NUM_P] = {"Prest", "Slot", "Fuent", "Model", "Octav", "Macro", "M>Gol", "M>Pro", "M>Azr", "M>Rot",
      "M>Den", "M>Sol", "M>Slt", "Loop", "Fijo", "Prior", "Trans", "Retar", "Estil", "Dens", "Solt", "CV2", "Reloj"};
    return N[i];
  }
  void Value(int i, char *b, bool full) {
    brains::Engine &e = brains_engine; brains::Settings &s = e.s; const brains::Slot &sl = s.slot[slot];
    const int m = full ? 9 : 5;
    switch (i) {
      case P_PRESET: if (!preset) xuzu::Copy(b, "---", m); else { xuzu::Itoa(preset, b); if (full) { const int l = strlen(b); xuzu::Copy(b + l, " cargado", 9 - l); } } break;
      case P_SLOT: xuzu::Itoa(slot + 1, b); break;
      case P_SRC: { static const char *const S[brains::NUM_SOURCES] = {"---", "E T1", "E T2", "E T3", "E T4", "C Acc", "C Rel", "C Baj", "C Mel"};
                    xuzu::Copy(b, full ? brains::SOURCE_NAMES[sl.source] : S[sl.source], m); break; }
      case P_MODEL: xuzu::Itoa(sl.model + 1, b); break;
      case P_OCT: xuzu::Signed(sl.oct, b); break;
      case P_MACRO: xuzu::Signed(s.macro_base, b); break;
      case P_MHITS: case P_MPROB: case P_MAZAR: case P_MROT: case P_MDENS: case P_MSOLT: case P_MSLOTS:
        xuzu::Signed(s.macro_amt[i - P_MHITS], b); break;
      case P_LOOPLEN: { static const char *const L[3] = {"1 c", "2 c", "4 c"}; xuzu::Copy(b, L[s.loop_len], m); break; }
      case P_FIXED: xuzu::Copy(b, e.loop_fixed ? "Si" : "No", m); break;
      case P_PRIO: xuzu::Copy(b, s.priority ? "Alta" : "Baja", m); break;
      case P_TRANS: xuzu::Itoa(s.transition * 20, b); if (full) { const int l = strlen(b); xuzu::Copy(b + l, " ms", 9 - l); } break;
      case P_DELAY: xuzu::Itoa(s.trig_delay, b); if (full) { const int l = strlen(b); xuzu::Copy(b + l, " ms", 9 - l); } break;
      case P_CSTYLE: xuzu::Copy(b, compas::STYLES[s.c_style].name, m); break;
      case P_CDENS: xuzu::Itoa(s.c_dens, b); break;
      case P_CSOLT: xuzu::Itoa(s.c_solt, b); break;
      case P_CV2: xuzu::Copy(b, cv2 ? (full ? "Transicion" : "Trans") : "Loop", m); break;
      case P_CLK: xuzu::Copy(b, e.clk_x2 ? "x2 Pam" : "x4 Pam", m); break;
    }
  }
  void Change(int i, int dir) {
    brains::Engine &e = brains_engine; brains::Settings &s = e.s; brains::Slot &sl = s.slot[slot];
    switch (i) {
      case P_PRESET: preset = constrain(preset + dir, 0, 16); if (preset > 0) xuzu::BrainsLoadPreset(preset - 1); break;
      case P_SLOT: slot = constrain(slot + dir, 0, 3); break;
      case P_SRC: sl.source = constrain(sl.source + dir, 0, brains::NUM_SOURCES - 1); break;
      case P_MODEL: sl.model = constrain(sl.model + dir, 0, 7); break;
      case P_OCT: sl.oct = constrain(sl.oct + dir, -3, 3); break;
      case P_MACRO: s.macro_base = constrain(s.macro_base + dir * 5, -100, 100); break;
      case P_MHITS: case P_MPROB: case P_MAZAR: case P_MROT: case P_MDENS: case P_MSOLT: case P_MSLOTS:
        s.macro_amt[i - P_MHITS] = constrain(s.macro_amt[i - P_MHITS] + dir * 5, -100, 100); break;
      case P_LOOPLEN: s.loop_len = constrain(s.loop_len + dir, 0, 2); break;
      case P_FIXED: e.loop_fixed = dir > 0; break;
      case P_PRIO: s.priority = dir > 0; break;
      case P_TRANS: s.transition = constrain(s.transition + dir, 0, 100); break;
      case P_DELAY: s.trig_delay = constrain(s.trig_delay + dir, 0, 20); break;
      case P_CSTYLE: s.c_style = constrain(s.c_style + dir, 0, compas::NUM_STYLES - 1); break;
      case P_CDENS: s.c_dens = constrain(s.c_dens + dir * 5, 0, 100); break;
      case P_CSOLT: s.c_solt = constrain(s.c_solt + dir * 5, 0, 100); break;
      case P_CV2: cv2 = dir > 0; break;
      case P_CLK: e.clk_x2 = dir > 0; break;
    }
  }

private:
  int preset = 0, slot = 0, cv2 = 0;
  int since = 1000000, last = 2000, flash = 0;
  static inline uint32_t last_owner_tick = 0xFFFFFFFFu;
};
