// Eucl x4 applet (xuzu rebuild of v4.16): plays the 4 tracks exactly as programmed in the Euclid x4 app.
// TR1 clock, TR2 reset. All 4 tracks go out over MIDI (ch 11-14); one track (T3 by default) goes to CV:
// output 1 trigger (or gate for note tracks), output 2 pitch. The encoder picks the CV track.
#pragma once
#include "xuzu/euclid4_engine.h"

class EuclX4 : public HemisphereApplet {
public:
  const char *applet_name() { return "Eucl x4"; }
  void Start() { cv_track = 2; }

  void Controller() {
    euclid4::Engine &e = euclid_engine;
    // with an Eucl x4 on each side, only one of them runs the shared engine
    const bool owner = (last_owner_tick != OC::CORE::ticks);
    if (owner) {
      last_owner_tick = OC::CORE::ticks;
      e.EndTick();          // clears the flags of the previous tick; the other side reads this tick's after us
      if (since < 1000000) since++;
      if (Clock(1)) e.Reset();
      if (Clock(0)) {
        if (since > 16667) { e.Reset(); e.Clock16(last); } else { e.Clock16(since); last = since; }
        since = 0;
      }
      e.Update();
    }
    const int k = cv_track;
    if (e.t[k].nota) GateOut(0, e.gate_left[k] > 0 || e.hit[k]);
    else if (e.hit[k]) ClockOut(0);
    Out(1, e.PitchCV(k));
    if (e.hit[k]) flash = 600;
    if (flash) flash--;
  }
  void View() {
    euclid4::Engine &e = euclid_engine;
    for (int t = 0; t < 4; ++t) {
      const int y = 15 + t * 10;
      gfxPrint(1, y, t + 1);
      const int L = e.Len(t), n = L < 16 ? L : 16, pos = e.StepPos(t), start = (pos / 16) * 16;
      for (int i = 0; i < n && start + i < L; ++i) {
        const int x = 9 + i * 3;
        if (e.PatternBit(t, (start + i) % e.Steps(t))) gfxRect(x, y + 1, 2, 5); else gfxPixel(x, y + 5);
      }
      if (t == cv_track) gfxInvert(0, y - 1, 8, 9);
    }
    gfxPrint(1, 55, "CV: pista "); gfxPrint(cv_track + 1);
    if (flash) gfxRect(58, 56, 4, 6);
  }
  void OnEncoderMove(int dir) { cv_track = constrain(cv_track + dir, 0, 3); }
  void OnButtonPress() {}
  uint64_t OnDataRequest() { uint64_t d = 0; Pack(d, PackLocation{0, 2}, cv_track); return d; }
  void OnDataReceive(uint64_t d) { cv_track = Unpack(d, PackLocation{0, 2}); }

protected:
  void SetHelp() {
    help[HELP_DIGITAL1] = "Clock";
    help[HELP_DIGITAL2] = "Reset";
    help[HELP_CV1] = "";
    help[HELP_CV2] = "";
    help[HELP_OUT1] = "Trig";
    help[HELP_OUT2] = "Tono";
    help[HELP_EXTRA1] = "Pistas de la app Euclid";
    help[HELP_EXTRA2] = "4 por MIDI, 1 por CV";
  }

private:
  int cv_track = 2, since = 1000000, last = 2000, flash = 0;
  static inline uint32_t last_owner_tick = 0xFFFFFFFFu;
};
