// Brains Seq (xuzu rebuild of v4.16): preset sequencer for Brains (Plaits clone).
//
// TR1 clock (x4 of the Pam)  TR2 reset  TR3 loop (gate)
// CV1 macro  CV2 C/D transition time (0 V = jump)  CV4 preset 1-16
// A trigger to Brains   B MODEL   C, D: MORPH / HARMONICS / TIMBRE / V/OCT, patch as you like
// Right encoder: slot. Long left: new material for the internal Compas. Long DOWN: fixed loop on/off.

#ifdef ENABLE_APP_BRAINSSEQ

#include "HSApplication.h"
#include "xuzu/brains_engine.h"
#include "xuzu/xuzu_flash.h"

brains::Engine brains_engine;

class BrainsSeq : public HSApplication {
public:
  struct __attribute__((packed)) CompactMat { uint32_t r0, m0, h; int8_t mp0; uint8_t pos, valid; };
  struct __attribute__((packed)) State { brains::Settings s; uint8_t calib[8]; uint8_t clk_x2, preset; };
  struct FlashPreset { xuzu::PresetHeader h; brains::Settings s; CompactMat mat; };
  static_assert(sizeof(FlashPreset) <= xuzu::PRESET_SIZE, "Brains preset too big");
  static_assert(sizeof(State) != 30, "must differ from the v4.16 layout");

  enum Param {
    SOURCE, MODEL, C_MODE, C_VAL, D_MODE, D_VAL, OCTN,
    PRESET, LOADP, SAVEP, EPRESET, SAVESTATE,
    C_STYLE, C_FAM, C_DENS, C_SOLT, C_NEW,
    MACRO, M_HITS, M_PROB, M_AZAR, M_ROT, M_DENS, M_SOLT, M_SLOTS,
    LOOP, LOOP_LEN, PRIORITY, TRANS, TDELAY, CLK,
    CAL1, CAL2, CAL3, CAL4, CAL5, CAL6, CAL7, CAL8,
    NUM_PARAMS };

  void Start() { E().Init(); }
  void Resume() {}
  void Suspend() {}
  brains::Engine &E() { return brains_engine; }

  void Controller() {
    brains::Engine &e = E();
    e.SetMacro(frame.inputs[0]);
    e.SetTransition(frame.inputs[1]);
    e.SetLoopGate(frame.gate_high[2]);
    { const int s = xuzu::CVPreset(frame.inputs[3], HSAPPLICATION_5V, cv_preset); if (s >= 0) { preset = s; LoadPreset(s); } }

    if (since_clock < 2000000) since_clock++;
    if (frame.clocked[1]) e.Reset();
    if (frame.clocked[0]) {
      if (since_clock > 16667) e.Reset();          // 1 s without clock: this one is the 1
      e.Clock(since_clock > 16667 ? last_period : since_clock);
      if (since_clock <= 16667) last_period = since_clock;
      since_clock = 0;
    }
    e.Update();
    if (calibrating >= 0) Out(1, brains::Engine::Calib(e.calib[calibrating]));
    else Out(1, e.out_model);
    Out(2, e.out_c);
    Out(3, e.out_d);
    if (e.trig) { ClockOut(0, HEMISPHERE_CLOCK_TICKS * 5); flash = 800; }
    if (flash) flash--;
    if (msg_timer) msg_timer--;
    if (calib_timer && --calib_timer == 0) calibrating = -1;
  }

  void View() {
    brains::Engine &e = E();
    gfxHeader("Brains Seq");
    gfxPrint(74, 1, PAGE_NAMES[page]);
    for (int i = 0; i < 4; ++i) {
      const int y = 13 + i * 8;
      gfxPrint(0, y, i + 1);
      gfxPrint(8, y, SHORT_SRC[e.s.slot[i].source]);
      gfxPrint(70, y, "M"); gfxPrint(e.s.slot[i].model + 1);
      gfxPrint(88, y, e.s.slot[i].c_note ? "C:n" : ""); gfxPrint(108, y, e.s.slot[i].d_note ? "D:n" : "");
      if (i >= e.ActiveSlots()) gfxPrint(0, y, "-");
      if (i == slot) gfxFrame(0, y - 1, 7, 9);
      if (e.last_slot == i && flash) gfxInvert(7, y - 1, 60, 9);
    }
    if (e.looping) gfxPrint(100, 1, "LOOP");
    gfxLine(0, 45, 127, 45);
    const int prm = PAGE_PARAMS[page][cursor];
    if (prm <= OCTN) { gfxPrint(0, 47, "S"); gfxPrint(slot + 1); gfxPrint(" "); } else gfxPrint(0, 47, "");
    gfxPrint(NAMES[prm]);
    gfxPrint(0, 56, "");
    if (msg_timer && (prm == SAVEP || prm == LOADP || prm == SAVESTATE || prm == PRESET)) gfxPrint(msg);
    else PrintValue(prm);
    gfxInvert(0, 46, 128, 18);
  }

  void OnButton(int control) {
    switch (control) {
    case OC::CONTROL_BUTTON_UP:   page = (page + NUM_PAGES - 1) % NUM_PAGES; cursor = 0; break;
    case OC::CONTROL_BUTTON_DOWN: page = (page + 1) % NUM_PAGES; cursor = 0; break;
    case OC::CONTROL_BUTTON_L:    cursor = (cursor + 1) % PAGE_LEN[page]; break;
    default: break;
    }
  }
  void OnLongPress(int control) {
    if (control == OC::CONTROL_BUTTON_DOWN) { E().loop_fixed = !E().loop_fixed; Message(E().loop_fixed ? "Loop fijo" : "Loop libre"); }
    if (control == OC::CONTROL_BUTTON_L) E().NewMaterial();
  }
  void OnLeftEncoderMove(int dir) { Edit(PAGE_PARAMS[page][cursor], dir); }
  void OnRightEncoderMove(int dir) { slot = constrain(slot + dir, 0, 3); }

  size_t SaveState(void *dst) {
    State *s = (State *)dst;
    s->s = E().s; for (int i = 0; i < 8; ++i) s->calib[i] = E().calib[i]; s->clk_x2 = E().clk_x2; s->preset = preset;
    return sizeof(State);
  }
  size_t RestoreState(const void *src) {
    const State *s = (const State *)src;
    if (s->s.slot[0].source >= brains::NUM_SOURCES) return sizeof(State);
    E().s = s->s; for (int i = 0; i < 8; ++i) E().calib[i] = s->calib[i]; E().clk_x2 = s->clk_x2 & 1; preset = s->preset & 15;
    return sizeof(State);
  }

private:
  static constexpr int NUM_PAGES = 6;
  static constexpr const char *PAGE_NAMES[NUM_PAGES] = {"Slots", "Presets", "Compas", "Macro CV1", "Loop / reloj", "Calibrar"};
  static constexpr int PAGE_LEN[NUM_PAGES] = {7, 5, 5, 8, 6, 8};
  static constexpr uint8_t PAGE_PARAMS[NUM_PAGES][8] = {
    {SOURCE, MODEL, C_MODE, C_VAL, D_MODE, D_VAL, OCTN},
    {PRESET, LOADP, SAVEP, EPRESET, SAVESTATE},
    {C_STYLE, C_FAM, C_DENS, C_SOLT, C_NEW},
    {MACRO, M_HITS, M_PROB, M_AZAR, M_ROT, M_DENS, M_SOLT, M_SLOTS},
    {LOOP, LOOP_LEN, PRIORITY, TRANS, TDELAY, CLK},
    {CAL1, CAL2, CAL3, CAL4, CAL5, CAL6, CAL7, CAL8}};
  static constexpr const char *NAMES[NUM_PARAMS] = {
    "Fuente", "Modelo", "Salida C", "Valor C", "Salida D", "Valor D", "Octava nota",
    "Preset", "Cargar preset", "Guardar preset", "Euclid preset", "Guardar estado",
    "Estilo", "Armonia", "Densidad", "Soltura", "Nuevo material",
    "Macro", "M: Golpes E", "M: Probab. E", "M: Azar E", "M: Rotacion E", "M: Densidad C", "M: Soltura C", "M: Slots",
    "Loop", "Largo loop", "Prioridad", "Transicion", "Retardo trig", "Reloj TR1",
    "Modelo 1 V", "Modelo 2 V", "Modelo 3 V", "Modelo 4 V", "Modelo 5 V", "Modelo 6 V", "Modelo 7 V", "Modelo 8 V"};
  static constexpr const char *SHORT_SRC[brains::NUM_SOURCES] = {"---", "Eucl T1", "Eucl T2", "Eucl T3", "Eucl T4", "C acent", "C relle", "C bajo", "C melod"};

  int page = 0, cursor = 0, slot = 0, preset = 0;
  int since_clock = 2000000, last_period = 2000, flash = 0, cv_preset = -1;
  int calibrating = -1, calib_timer = 0;
  const char *msg = ""; int msg_timer = 0;

  void Message(const char *m) { msg = m; msg_timer = 16667 * 2; }

  void SavePreset(int s) {
    FlashPreset fp; fp.s = E().s;
    compas::Engine::Material m; E().compas.GetMaterial(m);
    fp.mat.r0 = m.r_seed[0]; fp.mat.m0 = m.m_seed[0]; fp.mat.h = m.h_seed; fp.mat.mp0 = m.m_pos[0]; fp.mat.pos = m.pos;
    fp.mat.valid = (m.r_valid & 1) | ((m.m_valid & 1) << 1) | (m.h_valid ? 4 : 0);
    const bool ok = xuzu::SavePreset(xuzu::D_BRAINS, s, 1, &fp, sizeof(fp));
    OC::save_app_data();
    Message(ok ? "Guardado" : "Error flash");
  }
  void LoadPreset(int s) {
    FlashPreset fp;
    if (!xuzu::LoadPreset(xuzu::D_BRAINS, s, 1, &fp, sizeof(fp))) { Message("Vacio"); return; }
    E().s = fp.s;     // calibration and clock are not in presets
    compas::Engine::Material m;
    for (int i = 0; i < compas::MAX_LOOP; ++i) { m.r_seed[i] = fp.mat.r0; m.m_seed[i] = fp.mat.m0; m.m_pos[i] = fp.mat.mp0; }
    m.h_seed = fp.mat.h; m.entropy = fp.mat.r0 ^ fp.mat.m0 ^ fp.mat.h ^ 0x9E37u; m.pos = fp.mat.pos;
    m.r_valid = fp.mat.valid & 1; m.m_valid = (fp.mat.valid >> 1) & 1; m.h_valid = (fp.mat.valid >> 2) & 1;
    E().compas.style = fp.s.c_style; E().compas.fam = fp.s.c_fam;
    E().compas.SetMaterial(m);
#ifdef ENABLE_APP_EUCLID4
    if (fp.s.euclid_preset > 0) Euclid4_instance.LoadPresetSlot(fp.s.euclid_preset - 1);
#endif
    Message("Cargado");
  }

  uint8_t *Field(int prm) {
    brains::Settings &s = E().s; brains::Slot &sl = s.slot[slot];
    switch (prm) {
      case SOURCE: return &sl.source; case MODEL: return &sl.model; case C_MODE: return &sl.c_note; case C_VAL: return &sl.c_val;
      case D_MODE: return &sl.d_note; case D_VAL: return &sl.d_val; case OCTN: return (uint8_t *)&sl.oct;
      case EPRESET: return &s.euclid_preset;
      case C_STYLE: return &s.c_style; case C_FAM: return &s.c_fam; case C_DENS: return &s.c_dens; case C_SOLT: return &s.c_solt;
      case MACRO: return (uint8_t *)&s.macro_base;
      case M_HITS: case M_PROB: case M_AZAR: case M_ROT: case M_DENS: case M_SOLT: case M_SLOTS: return (uint8_t *)&s.macro_amt[prm - M_HITS];
      case LOOP_LEN: return &s.loop_len; case PRIORITY: return &s.priority; case TRANS: return &s.transition; case TDELAY: return &s.trig_delay;
      case CLK: return &E().clk_x2;
      default: if (prm >= CAL1 && prm <= CAL8) return &E().calib[prm - CAL1]; return nullptr;
    }
  }
  static bool Signed(int prm) { return prm == OCTN || prm == MACRO || (prm >= M_HITS && prm <= M_SLOTS); }
  void Range(int prm, int &lo, int &hi, int &st) {
    lo = 0; st = 1;
    switch (prm) {
      case SOURCE: hi = brains::NUM_SOURCES - 1; break; case MODEL: hi = 7; break;
      case C_MODE: case D_MODE: case PRIORITY: case CLK: case LOOP: hi = 1; break;
      case C_VAL: case D_VAL: case C_DENS: case C_SOLT: hi = 100; st = 5; break;
      case OCTN: lo = -3; hi = 3; break;
      case EPRESET: hi = 16; break;
      case C_STYLE: hi = compas::NUM_STYLES - 1; break; case C_FAM: hi = compas::NUM_FAMS - 1; break;
      case MACRO: case M_HITS: case M_PROB: case M_AZAR: case M_ROT: case M_DENS: case M_SOLT: case M_SLOTS: lo = -100; hi = 100; st = 5; break;
      case LOOP_LEN: hi = 2; break; case TRANS: hi = 100; break; case TDELAY: hi = 20; break;
      case PRESET: hi = 15; break;
      default: hi = 100; break;     // calibration: 0..5 V in 0.05 V
    }
  }
  int Get(int prm) {
    if (prm == PRESET) return preset;
    if (prm == LOOP) return E().loop_fixed;
    uint8_t *f = Field(prm); if (!f) return 0;
    return Signed(prm) ? (int)(int8_t)*f : *f;
  }
  void Edit(int prm, int dir) {
    switch (prm) {
      case LOADP: if (dir > 0) LoadPreset(preset); return;
      case SAVEP: if (dir > 0) SavePreset(preset); return;
      case SAVESTATE: if (dir > 0) { OC::save_app_data(); Message("Guardado"); } return;
      case C_NEW: if (dir > 0) E().NewMaterial(); return;
      default: break;
    }
    int lo, hi, st; Range(prm, lo, hi, st);
    const int v = constrain(Get(prm) + dir * st, lo, hi);
    if (prm == PRESET) { preset = v; return; }
    if (prm == LOOP) { E().loop_fixed = v; return; }
    if (uint8_t *f = Field(prm)) *f = (uint8_t)v;
    if (prm >= CAL1 && prm <= CAL8) { calibrating = prm - CAL1; calib_timer = 16667 * 3; }   // B plays it right away
  }
  void PrintValue(int prm) {
    const int v = Get(prm);
    switch (prm) {
      case SOURCE: gfxPrint(brains::SOURCE_NAMES[v]); break;
      case MODEL: gfxPrint(v + 1); gfxPrint(" ("); PrintVolts(E().calib[v]); gfxPrint(" V)"); break;
      case C_MODE: case D_MODE: gfxPrint(v ? "Nota (V/oct)" : "Valor"); break;
      case C_VAL: case D_VAL: gfxPrint(v); gfxPrint("% (0-5V)"); break;
      case OCTN: if (v > 0) gfxPrint("+"); gfxPrint(v); break;
      case PRESET: gfxPrint(v + 1); gfxPrint(xuzu::PresetValid(xuzu::D_BRAINS, v, 1) ? "" : " (vacio)"); break;
      case EPRESET: if (!v) gfxPrint("No cambia"); else { gfxPrint("Carga el "); gfxPrint(v); } break;
      case C_STYLE: gfxPrint(compas::STYLES[v].name); break;
      case C_FAM: gfxPrint(compas::FAMS[v].name); break;
      case MACRO: gfxPrint(v); gfxPrint(" + CV1"); break;
      case M_HITS: case M_PROB: case M_AZAR: case M_ROT: case M_DENS: case M_SOLT: case M_SLOTS: if (v > 0) gfxPrint("+"); gfxPrint(v); gfxPrint("%"); break;
      case LOOP: gfxPrint(v ? "Fijo" : "TR3 (gate)"); break;
      case LOOP_LEN: { static const char *const L[3] = {"1 compas", "2 compases", "4 compases"}; gfxPrint(L[v]); break; }
      case PRIORITY: gfxPrint(v ? "Slot mas alto" : "Slot mas bajo"); break;
      case TRANS: if (!v) gfxPrint("0 salto"); else { gfxPrint(v * 20); gfxPrint(" ms + CV2"); } break;
      case TDELAY: gfxPrint(v); gfxPrint(" ms"); break;
      case CLK: gfxPrint(v ? "x2 Pam" : "x4 Pam"); break;
      case LOADP: case SAVEP: case SAVESTATE: case C_NEW: gfxPrint("gira >"); break;
      default: PrintVolts(v); gfxPrint(" V  (sale en B)"); break;
    }
  }
  void PrintVolts(int u) { const int mv = u * 50; gfxPrint(mv / 1000); gfxPrint("."); const int c = (mv % 1000) / 10; if (c < 10) gfxPrint("0"); gfxPrint(c); }
};

constexpr const char *BrainsSeq::PAGE_NAMES[];
constexpr int BrainsSeq::PAGE_LEN[];
constexpr uint8_t BrainsSeq::PAGE_PARAMS[][8];
constexpr const char *BrainsSeq::NAMES[];
constexpr const char *BrainsSeq::SHORT_SRC[];

BrainsSeq BrainsSeq_instance;

void BrainsSeq_init() { BrainsSeq_instance.BaseStart(); }
static constexpr size_t BrainsSeq_storageSize() { return sizeof(BrainsSeq::State); }
size_t BrainsSeq_save(void *dst) { return BrainsSeq_instance.SaveState(dst); }
size_t BrainsSeq_restore(const void *src) { return BrainsSeq_instance.RestoreState(src); }
void BrainsSeq_isr() { BrainsSeq_instance.BaseController(); }
void BrainsSeq_handleAppEvent(OC::AppEvent event) {
  if (event == OC::APP_EVENT_RESUME) BrainsSeq_instance.Resume();
  if (event == OC::APP_EVENT_SUSPEND) BrainsSeq_instance.Suspend();
}
void BrainsSeq_loop() {}
void BrainsSeq_menu() { BrainsSeq_instance.BaseView(); }
void BrainsSeq_screensaver() { BrainsSeq_instance.BaseScreensaver(true); }
void BrainsSeq_handleButtonEvent(const UI::Event &event) {
  if (event.type == UI::EVENT_BUTTON_PRESS) BrainsSeq_instance.OnButton(event.control);
  if (event.type == UI::EVENT_BUTTON_LONG_PRESS) BrainsSeq_instance.OnLongPress(event.control);
}
void BrainsSeq_handleEncoderEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_ENCODER_L) BrainsSeq_instance.OnLeftEncoderMove(event.value);
  if (event.control == OC::CONTROL_ENCODER_R) BrainsSeq_instance.OnRightEncoderMove(event.value);
}

#endif // ENABLE_APP_BRAINSSEQ
