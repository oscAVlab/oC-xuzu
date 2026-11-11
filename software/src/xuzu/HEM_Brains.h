// Brains applet (xuzu rebuild of v4.16): plays the slots of the Brains Seq app.
// TR1 clock, TR2 reset.  CV1 macro, CV2 loop (gate).  Output 1 trigger to Brains, output 2 MODEL.
#pragma once
#include "xuzu/brains_engine.h"

class Brains : public HemisphereApplet {
public:
  const char *applet_name() { return "Brains"; }
  void Start() {}
  void Controller() {
    brains::Engine &e = brains_engine;
    const bool owner = (last_owner_tick != OC::CORE::ticks);
    if (owner) {
      last_owner_tick = OC::CORE::ticks;
      e.SetMacro(In(0));
      e.SetLoopGate(In(1) > HEMISPHERE_3V_CV / 2);
      e.SetTransition(0);
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
  void View() {
    brains::Engine &e = brains_engine;
    for (int i = 0; i < 4; ++i) {
      const int y = 15 + i * 10;
      gfxPrint(1, y, i + 1);
      gfxPrint(10, y, "M"); gfxPrint(e.s.slot[i].model + 1);
      if (i >= e.ActiveSlots()) gfxPrint(30, y, "-");
      if (e.last_slot == i && flash) gfxInvert(0, y - 1, 63, 9);
    }
    if (e.looping) gfxPrint(1, 55, "LOOP");
  }
  void OnEncoderMove(int dir) { (void)dir; }
  void OnButtonPress() {}
  uint64_t OnDataRequest() { return 0; }
  void OnDataReceive(uint64_t d) { (void)d; }

protected:
  void SetHelp() {
    help[HELP_DIGITAL1] = "Clock";
    help[HELP_DIGITAL2] = "Reset";
    help[HELP_CV1] = "Macro";
    help[HELP_CV2] = "LOOP";
    help[HELP_OUT1] = "Trig";
    help[HELP_OUT2] = "Model";
    help[HELP_EXTRA1] = "Slots: app Brains Seq";
    help[HELP_EXTRA2] = "";
  }

private:
  int since = 1000000, last = 2000, flash = 0;
  static inline uint32_t last_owner_tick = 0xFFFFFFFFu;
};
