// Compas (xuzu rebuild of v4.16): rhythm + harmony + melody generator for Ornament & Crime
// MIT License, (c) 2026 oscAVlab
//
// TR1 clock (x4 or x2 of the Pam)  TR2 reset (Ciclo / Rejilla)  TR3 remate  TR4 next style
// CV1-CV3 assignable (default Intensidad, Progresion, Loop congela)  CV4 preset 1-16
// A accents / 4x4 kick   B melody, bass or arpeggio (CV)   C fills + burst   D root, JF note or melody gate
// USB MIDI ch10: 35 kick 4x4, 36 grave, 38 agudo, 39 palma clara, 40 palma sorda,
//                41 pie grave, 42 pie agudo, 43 rafaga, 49 remate
//          ch1 melody   ch2 chords   ch3 bass.  MIDI clock out.
// UP / DOWN: page. Left encoder: press = next parameter, turn = edit, long = new material.
// Long DOWN: remate. Right encoder: turn = soltura, press = new material.

#ifdef ENABLE_APP_COMPAS

#include <math.h>
#include "HSApplication.h"
#include "xuzu/compas_engine.h"
#include "xuzu/xuzu_flash.h"

class Compas : public HSApplication {
public:
  enum Param {
    // ---- musical settings, stored in presets (keep this order: presets depend on it)
    STYLE, PERFIL, DENS, ADORNO, EMPUJE, REMATES, OUT_A,
    FAM, PROG, REHARM, FRASE,
    LOOP_R, LOOP_M, LOOP_H, SOLT, LOCK, VEL_R, VEL_M, SEGUIR,
    MEL_STYLE, NOTAS, RANGO, OCT_B, OCT_D, ORN_CV, OUT_B, OUT_D,
    B_MODE, B_STEPS, B_HITS, B_ROT, B_LEN, B_CURVE, B_PROB, B_OUT,
    CHORDS, BASS, VOICING,
    NUM_PRESET_PARAMS,
    // ---- rack settings, not in presets
    MIDI = NUM_PRESET_PARAMS, MCLK, CLK, CV1D, CV2D, CV3D, RESET_MODE,
    NUM_SAVED_PARAMS,
    // ---- actions
    DESPL = NUM_SAVED_PARAMS, PRESET, LOAD_ALL, LOAD_SET, SAVE,
    NUM_PARAMS };

  enum CvDest { CV_NADA, CV_ESTILO, CV_ARMONIA, CV_PROG, CV_DENS, CV_SOLT, CV_INTENS, CV_LOOPFRZ, CV_VELLOOP,
                CV_LOOP_R, CV_LOOP_M, CV_ADORNO, CV_NOTAS, CV_REHARM, CV_RAFAGA, CV_PRESET, NUM_CV_DEST };

  // what gets persisted (EEPROM state + flash presets)
  struct __attribute__((packed)) CompactMat { uint32_t r0, m0, h; int8_t mp0; uint8_t pos, valid; };   // 15 bytes
  struct __attribute__((packed)) EEPreset { int8_t v[NUM_PRESET_PARAMS]; CompactMat mat; };                             // EEPROM copy of presets 1-8
  struct FlashPreset { xuzu::PresetHeader h; compas::Engine::Material mat; int8_t v[NUM_PRESET_PARAMS]; };
  static_assert(sizeof(FlashPreset) <= xuzu::PRESET_SIZE, "Compas preset too big for its flash slot");
  struct __attribute__((packed)) State { int8_t v[NUM_SAVED_PARAMS]; uint8_t preset; CompactMat mat; EEPreset ee[8]; };
  static_assert(sizeof(State) != 576, "must differ from the v4.16 layout so old EEPROM data is skipped, not misread");

  void Start() {
    engine.Reset();
    engine.Seed(0xC0A5u ^ (micros() << 7));
    for (int i = 0; i < 8; ++i) ee[i].v[0] = -1;   // empty
  }
  void Resume() {}
  void Suspend() { AllNotesOff(); }

  // ------------------------------------------------------------------ ISR
  void Controller() {
    ApplyCV();

    if (frame.clocked[2]) engine.RequestRemate();
    if (frame.clocked[3]) p[STYLE] = WrapStyle(p[STYLE] + 1);

    if (since_clock < MAX_PERIOD) since_clock++;
    if (frame.clocked[1]) Reset();
    if (frame.clocked[0]) {
      if (skip_clocks > 0) { skip_clocks--; since_clock = 0; }
      else OnClock();
    }
    while (extra_clocks > 0) { extra_clocks--; OnClock(); }

    RunQueue();
    MidiClock();

    // B: melody / bass / arpeggio CV.  D: root, JF note or melody gate
    int b_cv = MelodyCV();
    if (p[OUT_B] == 1) b_cv = bass_semis * 128;
    else if (p[OUT_B] == 2) b_cv = arp_semis * 128;
    Out(1, b_cv + 12 * (p[OCT_B] - 4) * 128);
    if (glide_left > 0) --glide_left;
    if (p[OUT_D] == 2) GateOut(3, gate_d > 0);
    else if (p[OUT_D] == 3) GateOut(3, acc_d > 0);
    else Out(3, ((p[OUT_D] ? engine.JFSemis() : engine.RootSemis()) + 12 * (p[OCT_D] - 4)) * 128);
    if (gate_d > 0) --gate_d;
    if (acc_d > 0) --acc_d;
    if (flash_a) --flash_a;
    if (flash_c) --flash_c;
    if (msg_timer) --msg_timer;
  }

  // ------------------------------------------------------------------ UI
  void View() {
    gfxHeader("Compas");
    gfxPrint(80, 1, PAGE_NAMES[page]);
    DrawClock();

    const bool style_pending = engine.style != engine.active_style;
    gfxPrint(66, 14, style_pending ? ">" : "");
    gfxPrint(compas::STYLES[style_pending ? engine.style : engine.active_style].name);
    char chord[10]; engine.ChordName(chord);
    gfxPrint(66, 24, chord);
    gfxPrint(110, 24, NoteName(engine.melody_semis));
    if (engine.remate_cycle) gfxInvert(65, 22, 43, 10);
    gfxPrint(66, 34, engine.SelectionPending() ? ">" : "");
    gfxPrint(compas::FAMS[p[FAM]].abbr);
    gfxPrint(" ");
    gfxPrint(compas::Engine::ProgName(engine.ShownFam(), engine.fam == compas::H_AUTO ? engine.ShownProg() : engine.prog));

    // frozen layers
    if (layer[0].frozen) gfxPrint(1, 56, "R");
    if (layer[1].frozen) gfxPrint(7, 56, "M");
    if (pending_load >= 0) gfxPrint(1, 47, "P");

    gfxLine(64, 44, 127, 44);
    const int prm = PAGE_PARAMS[page][cursor];
    gfxPrint(66, 47, NAMES[prm]);
    gfxPrint(66, 56, "");
    if (msg_timer && (prm == SAVE || prm == LOAD_ALL || prm == LOAD_SET || prm == PRESET)) gfxPrint(msg);
    else PrintValue(prm);
    gfxInvert(65, 46, 62, 18);

    if (flash_a) gfxRect(1, 13, 4, 4);
    if (flash_c) gfxFrame(1, 19, 4, 4);
  }

  void OnButton(int control) {
    switch (control) {
    case OC::CONTROL_BUTTON_UP:   page = (page + NUM_PAGES - 1) % NUM_PAGES; cursor = 0; break;
    case OC::CONTROL_BUTTON_DOWN: page = (page + 1) % NUM_PAGES; cursor = 0; break;
    case OC::CONTROL_BUTTON_L:    cursor = (cursor + 1) % PAGE_LEN[page]; break;
    case OC::CONTROL_BUTTON_R:    engine.Reroll(); break;
    default: break;
    }
  }
  void OnLongPress(int control) {
    if (control == OC::CONTROL_BUTTON_DOWN) engine.RequestRemate();
    if (control == OC::CONTROL_BUTTON_L) engine.Reroll();
  }
  void OnLeftEncoderMove(int dir) { Edit(PAGE_PARAMS[page][cursor], dir); }
  void OnRightEncoderMove(int dir) { Edit(SOLT, dir); }

  // ------------------------------------------------------------------ storage
  size_t SaveState(void *dst) {
    State *s = (State *)dst;
    for (int i = 0; i < NUM_SAVED_PARAMS; ++i) s->v[i] = (int8_t)p[i];
    s->preset = (uint8_t)p[PRESET];
    Compact(s->mat);
    for (int i = 0; i < 8; ++i) s->ee[i] = ee[i];
    return sizeof(State);
  }
  size_t RestoreState(const void *src) {
    const State *s = (const State *)src;
    if (s->v[STYLE] < 0 || s->v[STYLE] >= compas::NUM_STYLES) return sizeof(State);   // never saved
    for (int i = 0; i < NUM_SAVED_PARAMS; ++i) p[i] = constrain(s->v[i], MIN_[i], MAX_[i]);
    p[PRESET] = constrain(s->preset, 0, xuzu::NUM_PRESETS - 1);
    for (int i = 0; i < 8; ++i) ee[i] = s->ee[i];
    PushParams();
    Uncompact(s->mat);
    return sizeof(State);
  }

private:
  static constexpr int NUM_PAGES = 8;
  static constexpr const char *PAGE_NAMES[NUM_PAGES] = {"Ritmo", "Armonia", "Loops", "Melodia", "Rafaga", "MIDI", "Entradas", "Presets"};
  static constexpr int PAGE_LEN[NUM_PAGES] = {8, 4, 8, 8, 8, 6, 4, 4};
  static constexpr uint8_t PAGE_PARAMS[NUM_PAGES][8] = {
    {STYLE, PERFIL, DENS, ADORNO, EMPUJE, REMATES, OUT_A, DESPL},
    {FAM, PROG, REHARM, FRASE},
    {LOOP_R, LOOP_M, LOOP_H, SOLT, LOCK, VEL_R, VEL_M, SEGUIR},
    {MEL_STYLE, NOTAS, RANGO, OCT_B, OCT_D, ORN_CV, OUT_B, OUT_D},
    {B_MODE, B_STEPS, B_HITS, B_ROT, B_LEN, B_CURVE, B_PROB, B_OUT},
    {MIDI, CHORDS, BASS, VOICING, MCLK, CLK},
    {CV1D, CV2D, CV3D, RESET_MODE},
    {PRESET, LOAD_ALL, LOAD_SET, SAVE}};
  static constexpr const char *NAMES[NUM_PARAMS] = {
    "Estilo", "Perfil", "Densidad", "Adorno", "Empuje", "Remates", "Salida A",
    "Armonia", "Progresion", "Rearmonizar", "Frase azar",
    "Loop ritmo", "Loop melodia", "Loop armonia", "Soltura", "Congelar", "Vel. ritmo", "Vel. melodia", "Seguir acorde",
    "Melodia", "Notas", "Rango", "Octava B", "Octava D", "Adornos CV", "Salida B", "Salida D",
    "Disparo", "Pasos", "Golpes", "Rotacion", "Largo", "Curva", "Probab.", "Destino",
    "Acordes", "Bajo", "Voicing",
    "MIDI USB", "Clock MIDI", "Reloj TR1", "CV1", "CV2", "CV3", "Reset TR2",
    "Desplazar", "Preset", "Cargar todo", "Cargar ajustes", "Guardar"};
  static constexpr int MIN_[NUM_PARAMS] = {
    0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 4, 0, 0, 0, 0, 0,
    0, 2, 1, 0, 0, 0, 0, 0,   0, 0, 0,   0, 0, 0, 0, 0, 0, 0,   -99, 0, 0, 0, 0};
  static constexpr int MAX_[NUM_PARAMS] = {
    compas::NUM_STYLES - 1, 1, 100, 100, 100, 1, 2,
    compas::NUM_FAMS - 1, compas::MAX_PROGS, 3, 1,
    compas::NUM_LOOPS - 1, compas::NUM_LOOPS - 1, 3, 100, compas::NUM_LOCKS - 1, 7, 7, 1,
    1, 100, 21, 6, 6, 1, 2, 3,
    5, 16, 16, 15, 3, 3, 100, 2,
    2, 4, 1,
    1, 1, 1, NUM_CV_DEST - 1, NUM_CV_DEST - 1, NUM_CV_DEST - 1, 1,
    99, xuzu::NUM_PRESETS - 1, 1, 1, 1};
  static constexpr int STEP_[NUM_PARAMS] = {
    1, 1, 5, 5, 5, 1, 1,   1, 1, 1, 1,   1, 1, 1, 5, 1, 1, 1, 1,   1, 5, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 5, 1,   1, 1, 1,   1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1};
  int p[NUM_PARAMS] = {
    0, 0, 55, 30, 20, 1, 0,   0, 1, 0, 0,   4, 5, 3, 15, 0, 3, 3, 0,   0, 50, 10, 4, 4, 1, 0, 0,
    0, 8, 5, 0, 1, 2, 100, 0,   0, 0, 0,   1, 0, 0, CV_INTENS, CV_PROG, CV_LOOPFRZ, 0,   0, 0, 0, 0, 0};
  int page = 0, cursor = 0;

  compas::Engine engine;
  EEPreset ee[8];
  static constexpr int MAX_PERIOD = 100000;
  static constexpr int PULSE = HEMISPHERE_CLOCK_TICKS * 5;
  static constexpr int DRUM_LEN = HEMISPHERE_CLOCK_TICKS * 25;
  static constexpr int STRUM_STEP = HEMISPHERE_CLOCK_TICKS * 9;
  static constexpr int LATE_RESET = HEMISPHERE_CLOCK_TICKS * 6;    // a reset this late still means "that clock"
  static constexpr int STOPPED = HEMISPHERE_CLOCK_TICKS * 1000;    // 1 s without clock: next clock is the 1
  int since_clock = MAX_PERIOD, period = MAX_PERIOD, tick_period = MAX_PERIOD, clock_count = 0;
  uint32_t raw = 0;
  int skip_clocks = 0, extra_clocks = 0;
  int flash_a = 0, flash_c = 0, gate_d = 0, acc_d = 0;
  int glide_left = 0, glide_len = 1, glide_from = 0;   // acid slide on output B, in DAC units
  bool slide_next = false;
  int melody_out = 12, bass_semis = 5, arp_semis = 5, arp_i = 0, carp_i = 0;
  int cv_last[4] = {0, 0, 0, 0};
  int cv_preset = -1;
  bool cv_freeze = false; int cv_loop = -1, cv_vel = -1; bool cv_burst_gate[3] = {false, false, false};
  int8_t mel_note = -1, bass_note = -1, carp_note = -1;
  int8_t chord_on[4] = {-1, -1, -1, -1};
  bool started = false;
  int mclk_left = 0, mclk_timer = 0, mclk_step = 0;
  uint32_t rng = 0x1234567u;
  int pending_load = -1; bool pending_with_material = true;
  const char *msg = ""; int msg_timer = 0;

  // ---------------------------------------------------------------- literal loop memory
  // Congelar records what just played and repeats it literally, at its own speed, while the
  // generator keeps running underneath.
  struct RStep {
    uint8_t fine_a, vel_a, fine_b, vel_b[compas::FINE], vmask[compas::NUM_EXTRA_VOICES], vvel[compas::NUM_EXTRA_VOICES];
    uint8_t comp, comp_vel, push, flags;
  };
  struct MStep { uint8_t mel_n, mel_main, mel_at[4]; int8_t mel_semis[4]; int8_t root; uint8_t notes, acc, tie; };
  enum { RF_STRUM = 1, RF_REMATE = 2, RF_BURST = 4, RF_BEAT = 8, RF_CYCLE = 16, RF_CHORD = 32 };
  static constexpr int HIST = 192;   // 8 cycles of the longest style (24 corcheas)
  RStep hist_r[HIST];
  MStep hist_m[HIST];
  struct Layer { bool frozen = false; int w = 0, count = 0, start = 0, len = 1, ph = 0; };   // ph in 1/den fine steps
  Layer layer[2];
  static constexpr uint8_t VEL_NUM[8] = {1, 2, 3, 1, 3, 2, 3, 4};
  static constexpr uint8_t VEL_DEN[8] = {2, 3, 4, 1, 2, 1, 1, 1};

  enum { Q_A = 1, Q_C, Q_VOICE, Q_OFF, Q_MEL, Q_MELORN, Q_CHORD, Q_CNOTE, Q_TICK, Q_BURST, Q_GATE };
  enum { MF_ACC = 0x100, MF_TIE = 0x200 };   // flags on a Q_MEL argument
  static constexpr int QN = 96;
  int q_time[QN] = {0};
  int16_t q_arg[QN] = {0};
  uint8_t q_type[QN] = {0}, q_vel[QN] = {0};
  static constexpr uint8_t VOICE_NOTE[compas::NUM_EXTRA_VOICES] = {39, 40, 41, 42};

  uint32_t Rand() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

  // ---------------------------------------------------------------- CV inputs
  int CvVolts10(int cv) const { return cv * 50 / HSAPPLICATION_5V; }   // tenths of a volt
  void ApplyCV() {
    int add_dens = 0, add_solt = 0, add_adorno = 0, add_notas = 0, add_style = 0, add_prog = 0, add_reharm = 0;
    int fam = p[FAM];
    bool freeze = false; int loopsel = -1, velsel = -1;
    for (int i = 0; i < 3; ++i) {
      const int cv = frame.inputs[i];
      switch (p[CV1D + i]) {
        case CV_ESTILO:  add_style = Sticky(cv, compas::NUM_STYLES, cv_last[i]); break;
        case CV_ARMONIA: fam = (p[FAM] + Sticky(cv, compas::NUM_FAMS, cv_last[i])) % compas::NUM_FAMS; break;
        case CV_PROG:    add_prog = Sticky(cv, compas::Engine::NumProgs(p[FAM]), cv_last[i]); break;
        case CV_DENS:    add_dens += Scale(cv, 100); break;
        case CV_SOLT:    add_solt += Scale(cv, 100); break;
        case CV_INTENS: {
          const int k = Scale(cv, 100);
          add_dens += k * 6 / 10; add_notas += k / 2; add_adorno += k / 2; add_solt += k * 4 / 10;
          break;
        }
        case CV_LOOPFRZ:
          if (CvVolts10(cv) > 4) { freeze = true; loopsel = constrain((cv - HSAPPLICATION_5V * 4 / 50) * 8 / (HSAPPLICATION_5V * 46 / 50), 0, 7); }
          break;
        case CV_VELLOOP: velsel = Sticky(cv, 8, cv_last[i]); break;
        case CV_LOOP_R:  loopsel = -2 - Sticky(cv, compas::NUM_LOOPS, cv_last[i]); break;   // handled below
        case CV_LOOP_M:  cv_loop_m = Sticky(cv, compas::NUM_LOOPS, cv_last[i]); break;
        case CV_ADORNO:  add_adorno += Scale(cv, 100); break;
        case CV_NOTAS:   add_notas += Scale(cv, 100); break;
        case CV_REHARM:  add_reharm = Sticky(cv, 4, cv_last[i]); break;
        case CV_RAFAGA: {
          const bool g = cv > HSAPPLICATION_5V / 2;
          if (g && !cv_burst_gate[i]) burst_req = true;
          cv_burst_gate[i] = g;
          break;
        }
        case CV_PRESET: { const int s = xuzu::CVPreset(cv, HSAPPLICATION_5V, cv_last[i]); if (s >= 0) RequestLoad(s, true); break; }
        default: break;
      }
    }
    // CV4: preset 1-16
    { const int s = xuzu::CVPreset(frame.inputs[3], HSAPPLICATION_5V, cv_preset); if (s >= 0) RequestLoad(s, true); }

    engine.style = WrapStyle(p[STYLE] + add_style);
    engine.fam = fam;
    const int np = compas::Engine::NumProgs(fam);
    engine.prog = (constrain(p[PROG], 0, np - 1) + add_prog) % np;
    engine.reharm = constrain(p[REHARM] + add_reharm, 0, 3);
    engine.density = constrain(p[DENS] + add_dens, 0, 100);
    engine.soltura = constrain(p[SOLT] + add_solt, 0, 100);
    engine.adorno = constrain(p[ADORNO] + add_adorno, 0, 100);
    engine.notas = constrain(p[NOTAS] + add_notas, 0, 100);
    engine.empuje = p[EMPUJE];
    engine.frase = p[FRASE] ? 8 : 4;
    engine.range = p[RANGO];
    engine.mel_style = p[MEL_STYLE];
    engine.perfil = p[PERFIL];
    engine.remates = p[REMATES];
    engine.loop_h = p[LOOP_H];
    // harmony freezing stays a seed lock; rhythm and melody are frozen literally by the app
    engine.lock = (p[LOCK] == compas::L_ARMONIA || p[LOCK] == compas::L_TODO) ? compas::L_ARMONIA : compas::L_NADA;

    int lr = p[LOOP_R], lm = p[LOOP_M];
    if (loopsel <= -2) lr = -2 - loopsel;
    if (cv_loop_m >= 0) { lm = cv_loop_m; cv_loop_m = -1; }
    cv_freeze = freeze;
    if (freeze) { lr = lm = loopsel; }
    engine.loop_r = lr; engine.loop_m = lm;
    cv_vel = velsel;
  }
  int cv_loop_m = -1;
  bool burst_req = false;

  bool FreezeR() const { return cv_freeze || p[LOCK] == compas::L_RITMO || p[LOCK] == compas::L_RIT_MEL || p[LOCK] == compas::L_TODO; }
  bool FreezeM() const { return cv_freeze || p[LOCK] == compas::L_MELODIA || p[LOCK] == compas::L_RIT_MEL || p[LOCK] == compas::L_TODO; }
  int VelIdx(int l) const { return cv_vel >= 0 ? cv_vel : p[l == 0 ? VEL_R : VEL_M]; }

  // ---------------------------------------------------------------- clock
  void Reset() {
    if (p[RESET_MODE] == 1) {          // Rejilla: realign the semicorchea grid and 4x4 kick only
      raw = 0; clock_count = 0;
      return;
    }
    // Ciclo. A reset that arrives a few ms after a clock refers to that clock: run it again as the 1
    const bool late = since_clock <= LATE_RESET && last_tick_from_clock;
    engine.Realign(); clock_count = 0; raw = 0;
    if (late) {
      for (int i = 0; i < QN; ++i) if (q_type[i] != Q_OFF) q_type[i] = 0;
      clock_count = 0;
      Tick(period);
      raw = 1;
    }
  }
  bool last_tick_from_clock = false;

  void OnClock() {
    const bool stopped = since_clock >= STOPPED;
    period = stopped ? MAX_PERIOD : since_clock;
    since_clock = 0;
    if (stopped) { engine.Realign(); clock_count = 0; raw = 0; }   // auto-align when the Pam starts
    RawClock();
    last_tick_from_clock = false;
    const bool flam = engine.Def().flags & compas::F_CELLS;
    const bool x4 = p[CLK] == 0;
    if ((x4 && flam) || (!x4 && !flam)) {
      Tick(period); last_tick_from_clock = true;
    } else if (x4 && !flam) {
      if ((clock_count++ & 1) == 0) { Tick(period < MAX_PERIOD / 2 ? period * 2 : MAX_PERIOD); last_tick_from_clock = true; }
    } else {
      Tick(period < MAX_PERIOD ? period / 2 : MAX_PERIOD); last_tick_from_clock = true;
      if (period < MAX_PERIOD) Enqueue(period / 2, Q_TICK, 0, 0);
    }
  }

  void RunQueue() {
    for (int i = 0; i < QN; ++i) {
      if (!q_type[i] || --q_time[i] > 0) continue;
      const uint8_t t = q_type[i]; q_type[i] = 0;
      switch (t) {
        case Q_A: FireA(q_vel[i]); break;
        case Q_C: FireC(q_vel[i]); break;
        case Q_VOICE: Drum(VOICE_NOTE[q_arg[i]], q_vel[i]); break;
        case Q_OFF: if (p[MIDI]) usbMIDI.sendNoteOff(q_arg[i] & 0x7F, 0, q_arg[i] >> 8); break;
        case Q_MEL: SetMelody((int8_t)(q_arg[i] & 0xFF), q_vel[i], true, q_arg[i] & MF_TIE); break;
        case Q_GATE: gate_d = q_arg[i]; if (q_vel[i]) acc_d = q_arg[i]; break;
        case Q_MELORN: SetMelody(q_arg[i], q_vel[i], false); break;
        case Q_CHORD: ChordHit(q_vel[i], q_arg[i]); break;
        case Q_CNOTE: if (p[MIDI]) usbMIDI.sendNoteOn(q_arg[i], q_vel[i], 2); break;
        case Q_TICK: Tick(tick_period); break;
        case Q_BURST: BurstHit(q_vel[i]); break;
      }
    }
  }
  void Enqueue(int delay, uint8_t type, int16_t arg, uint8_t vel) {
    if (delay <= 0) delay = 1;
    for (int i = 0; i < QN; ++i) if (!q_type[i]) { q_time[i] = delay; q_type[i] = type; q_arg[i] = arg; q_vel[i] = vel; return; }
  }

  // things that follow the Pam grid directly: 4x4 kick, techno bass, arpeggios, MIDI clock
  void RawClock() {
    const bool x4 = p[CLK] == 0;
    const int per_beat = x4 ? 4 : 2;
    const int in_beat = raw % per_beat;
    if (p[OUT_A] > 0 && in_beat == 0) {
      ClockOut(0, PULSE); flash_a = 1200;
      if (p[MIDI]) { usbMIDI.sendNoteOn(35, 120, 10); Enqueue(DRUM_LEN, Q_OFF, (int16_t)(35 | (10 << 8)), 0); }
    }
    if (BassMode() == 3 && in_beat == per_beat / 2) Bass(engine.RootSemis() % 12, 100);
    int8_t iv[4]; const int n = engine.ChordTones(iv);
    if (engine.tick == 0 && in_beat == 0) { arp_i = 0; carp_i = 0; }   // arpeggios start with the cycle
    if (BassMode() == 4) { const int k = arp_i++ % (n * 2); Bass(engine.RootSemis() % 12 + iv[k % n] + 12 * (k / n), 90); }
    const int k2 = carp_i++ % (n * 2);
    arp_semis = engine.RootSemis() % 12 + iv[k2 % n] + 12 * (k2 / n);
    if (p[CHORDS] == 2 && p[MIDI]) {
      if (carp_note >= 0) usbMIDI.sendNoteOff(carp_note, 0, 2);
      carp_note = (int8_t)constrain(64 + arp_semis, 0, 127);
      usbMIDI.sendNoteOn(carp_note, 85, 2);
    }
    if (p[MCLK] && p[MIDI]) {
      if (!started) { usbMIDI.sendRealTime(usbMIDI.Start); started = true; }
      mclk_left = x4 ? 6 : 12;
      mclk_step = period < MAX_PERIOD ? period / mclk_left : 0;
      mclk_timer = 0;
    }
    raw++;
  }
  void MidiClock() {
    if (mclk_left <= 0) return;
    if (--mclk_timer > 0) return;
    usbMIDI.sendRealTime(usbMIDI.Clock);
    mclk_left--;
    if (mclk_step == 0) mclk_left = 0;
    mclk_timer = mclk_step > 0 ? mclk_step : 1;
  }
  int BassMode() const { return (p[BASS] == 0 && p[OUT_B] == 1) ? 1 : p[BASS]; }
  void Bass(int semis, uint8_t vel) {
    bass_semis = semis;
    if (!p[MIDI] || p[BASS] == 0) return;
    if (bass_note >= 0) usbMIDI.sendNoteOff(bass_note, 0, 3);
    bass_note = (int8_t)constrain(40 + semis, 0, 127);
    usbMIDI.sendNoteOn(bass_note, vel, 3);
  }

  // ---------------------------------------------------------------- one corchea
  void Tick(int tp) {
    tick_period = tp;
    // a preset waiting for the start of the cycle
    if (pending_load >= 0 && (engine.tick < 0 || engine.tick + 1 >= engine.Def().len)) ApplyPendingLoad();

    const compas::Event ev = engine.Tick();
    const bool timed = tp < MAX_PERIOD;

    // live rhythm and melody of this corchea, in loop-memory form
    RStep rs; MStep ms;
    rs.fine_a = ev.fine_a; rs.vel_a = ev.remate ? 127 : ev.vel_a; rs.fine_b = ev.fine_b;
    for (int k = 0; k < compas::FINE; ++k) rs.vel_b[k] = ev.vel_b[k];
    for (int v = 0; v < compas::NUM_EXTRA_VOICES; ++v) { rs.vmask[v] = ev.vmask[v]; rs.vvel[v] = ev.vvel[v]; }
    rs.comp = ev.comp; rs.comp_vel = ev.comp_vel; rs.push = ev.push;
    rs.flags = (ev.strum ? RF_STRUM : 0) | (ev.remate ? RF_REMATE : 0) | (ev.beat_start ? RF_BEAT : 0) |
               (ev.cycle_start ? RF_CYCLE : 0) | (ev.new_chord ? RF_CHORD : 0) | (BurstDecision(ev) ? RF_BURST : 0);
    ms.mel_n = ev.mel_n; ms.mel_main = ev.mel_main; ms.root = (int8_t)engine.RootSemis();
    for (int i = 0; i < 4; ++i) { ms.mel_at[i] = ev.mel_at[i]; ms.mel_semis[i] = ev.mel_semis[i]; }
    ms.notes = ev.mel_notes | ((engine.Def().flags & compas::F_LINE16) ? 0x80 : 0);   // bit 7: a semicorchea line
    ms.acc = ev.mel_acc; ms.tie = ev.mel_tie;

    UpdateLayer(0, FreezeR(), engine.LoopTicks(engine.loop_r));
    UpdateLayer(1, FreezeM(), engine.LoopTicks(engine.loop_m));
    if (!layer[0].frozen) Record(0, &rs, nullptr);
    if (!layer[1].frozen) Record(1, nullptr, &ms);

    // rhythm
    if (layer[0].frozen) PlayFrozen(0, tp, timed);
    else EmitR(rs, tp, timed, 0, 1, 1);
    // melody
    if (layer[1].frozen) PlayFrozen(1, tp, timed);
    else EmitM(ms, tp, timed, 0, 1, 1);

    if (ev.new_chord && p[CHORDS] == 1) ChordHit(80, false);
    if (burst_req) { burst_req = false; Burst(tp, timed, true); }
  }

  void Record(int l, const RStep *r, const MStep *m) {
    Layer &L = layer[l];
    if (r) hist_r[L.w] = *r; else hist_m[L.w] = *m;
    L.w = (L.w + 1) % HIST;
    if (L.count < HIST) L.count++;
  }
  void UpdateLayer(int l, bool want, int loop_ticks) {
    Layer &L = layer[l];
    int len = loop_ticks < L.count ? loop_ticks : L.count;
    if (len < 1) len = 1;
    if (want && !L.frozen) {
      if (L.count == 0) return;
      L.frozen = true; L.len = len; L.start = (L.w - len + HIST) % HIST; L.ph = 0;
    } else if (!want && L.frozen) {
      L.frozen = false;
    } else if (L.frozen && len != L.len) {
      // while frozen, the loop length picks how much of what just played repeats
      L.len = len; L.start = (L.w - len + HIST) % HIST; L.ph = 0;
    }
  }
  // plays one corchea of a frozen layer at its speed: recorded fine step x lands at
  // real time (x - ph) / speed
  void PlayFrozen(int l, int tp, bool timed) {
    Layer &L = layer[l];
    const int vi = VelIdx(l);
    const int num = VEL_NUM[vi], den = VEL_DEN[vi];
    const int total = L.len * compas::FINE * den;      // loop length in 1/den fine steps
    const int span = compas::FINE * num;               // how much of the loop one corchea covers
    // walk the recorded fine steps in [ph, ph + span)
    for (int u = 0; u < span; ++u) {
      const int pos = (L.ph + u) % total;
      if (pos % den) continue;
      const int x = pos / den;                       // recorded fine step
      const int step = x / compas::FINE, k = x % compas::FINE;
      const int delay = timed ? tp * u / span : 0;
      if (!timed && u > 0) break;
      const int idx = (L.start + step) % HIST;
      if (l == 0) EmitR(hist_r[idx], tp, timed, k, num, den, delay);
      else EmitM(hist_m[idx], tp, timed, k, num, den, delay);
    }
    L.ph = (L.ph + span) % total;
  }

  // emits the part of a rhythm step that falls on fine step k (or all of it when live)
  void EmitR(const RStep &rs, int tp, bool timed, int k_only, int num, int den, int delay0 = 0) {
    const bool live = (num == 1 && den == 1 && delay0 == 0 && k_only == 0 && !layer[0].frozen);
    const int push = timed ? tp * rs.push / 100 : 0;
    for (int k = 0; k < compas::FINE; ++k) {
      if (!live && k != k_only) continue;
      if (live && k > 0 && !timed) continue;
      const int d = live ? (timed ? tp * k / compas::FINE : 0) : delay0;
      if (rs.fine_a & (1u << k)) { if (d == 0) FireA(rs.vel_a); else Enqueue(d, Q_A, 0, rs.vel_a); }
      if (rs.fine_b & (1u << k)) { const int dd = d + push; if (dd <= 0) FireC(rs.vel_b[k]); else Enqueue(dd, Q_C, 0, rs.vel_b[k]); }
      for (int v = 0; v < compas::NUM_EXTRA_VOICES; ++v) {
        if (!(rs.vmask[v] & (1u << k))) continue;
        const int dd = d + (v == compas::V_PIE_AGUDO ? push : 0);
        if (dd <= 0) Drum(VOICE_NOTE[v], rs.vvel[v]); else Enqueue(dd, Q_VOICE, v, rs.vvel[v]);
      }
      if (p[CHORDS] == 0 && (rs.comp & (1u << k))) {
        const bool strum = rs.flags & RF_STRUM;
        if (d == 0) ChordHit(rs.comp_vel, strum); else Enqueue(d, Q_CHORD, strum, rs.comp_vel);
      }
      if (k == 0) {
        const int bm = BassMode();
        if ((bm == 1 || bm == 2) && (rs.fine_a & 1u)) Bass(engine.RootSemis() % 12, rs.vel_a);
        if (bm == 2 && (rs.fine_b & 1u) && !(rs.fine_a & 1u)) Bass(engine.RootSemis() % 12 + 12, 80);
        if (rs.flags & RF_REMATE) Drum(49, 110);
        if (rs.flags & RF_BURST) Burst(tp * den / num, timed, false, d);
      }
    }
  }
  void EmitM(const MStep &ms, int tp, bool timed, int k_only, int num, int den, int delay0 = 0) {
    const bool live = (num == 1 && den == 1 && delay0 == 0 && k_only == 0 && !layer[1].frozen);
    int shift = 0;
    if (!live && p[SEGUIR]) { shift = ((engine.RootSemis() - ms.root) % 12 + 12) % 12; if (shift > 6) shift -= 12; }
    for (int i = 0; i < ms.mel_n; ++i) {
      const int k = ms.mel_at[i];
      if (!live && k != k_only) continue;
      const bool main = (ms.notes >> i) & 1;
      const bool acc = (ms.acc >> i) & 1, tie = (ms.tie >> i) & 1;
      const uint8_t vel = acc ? 122 : main ? 95 : 70;
      const int semis = ms.mel_semis[i] + shift;
      const int d = live ? (timed ? tp * k / compas::FINE : 0) : delay0;
      if (!timed && k > 0 && live) continue;
      if (d == 0) SetMelody(semis, vel, main, tie);
      else Enqueue(d, main ? Q_MEL : Q_MELORN, (int16_t)((semis & 0xFF) | (tie ? MF_TIE : 0)), vel);
      if (main) {
        // semicorchea notes (acid) get a short gate, or one that reaches the next note when tied
        const bool sixteenth = ms.notes & 0x80;
        const int step = sixteenth ? tp / 2 : tp;
        int len = timed ? (tie ? step + step / 4 : step * 6 / 10) : HEMISPHERE_CLOCK_TICKS * 20;
        if (len > 32000) len = 32000;
        if (d == 0) { gate_d = len; if (acc) acc_d = len; }
        else Enqueue(d, Q_GATE, (int16_t)len, acc ? 1 : 0);
      }
    }
  }

  // ---------------------------------------------------------------- Euclidean burst
  bool BurstDecision(const compas::Event &ev) {
    bool fire = false;
    switch (p[B_MODE]) {
      case 1: fire = ev.remate; break;
      case 2: fire = ev.fine_a & 1u; break;
      case 3: fire = ev.beat_start; break;
      case 4: fire = ev.cycle_start; break;
      case 5: fire = (int)(Rand() % 100) < 8 + engine.density / 8; break;
      default: return false;
    }
    return fire && (int)(Rand() % 100) < p[B_PROB];
  }
  void Burst(int tp, bool timed, bool forced, int delay0 = 0) {
    if (!timed) return;
    (void)forced;
    static const int8_t LEN_HALVES[4] = {1, 2, 4, 8};
    const int window = tp * engine.Def().beat * LEN_HALVES[p[B_LEN]] / 2;
    const int steps = p[B_STEPS], hits = p[B_HITS] < steps ? p[B_HITS] : steps;
    for (int i = 0; i < steps; ++i) {
      const int j = (i + p[B_ROT]) % steps;
      if ((j * hits) % steps >= hits) continue;
      const int x = steps > 1 ? i * 100 / (steps - 1) : 0;
      int vel;
      switch (p[B_CURVE]) {
        case 1: vel = 50 + x * 70 / 100; break;
        case 2: vel = 120 - x * 70 / 100; break;
        case 3: vel = 120 - (x > 50 ? x - 50 : 50 - x) * 7 / 5; break;
        default: vel = 100; break;
      }
      vel = constrain(vel, 1, 127);
      const int d = delay0 + window * i / steps;
      if (d == 0) BurstHit((uint8_t)vel);
      else Enqueue(d, Q_BURST, 0, (uint8_t)vel);
    }
  }
  void BurstHit(uint8_t vel) {
    if (p[B_OUT] != 1) { ClockOut(2, PULSE); flash_c = 1200; }
    if (p[B_OUT] != 0) Drum(43, vel);
  }

  // ---------------------------------------------------------------- outputs
  void FireA(uint8_t vel) {
    if (p[OUT_A] != 1) { ClockOut(0, PULSE); flash_a = 1200; }
    Drum(36, vel);
  }
  void FireC(uint8_t vel) { ClockOut(2, PULSE); flash_c = 1200; Drum(38, vel ? vel : 80); }
  void Drum(uint8_t note, uint8_t vel) {
    if (!p[MIDI]) return;
    usbMIDI.sendNoteOn(note, vel, 10);
    Enqueue(DRUM_LEN, Q_OFF, (int16_t)(note | (10 << 8)), 0);
  }
  int MelodyCV() const {
    const int to = melody_out * 128;
    return glide_left > 0 ? to + (glide_from - to) * glide_left / glide_len : to;
  }
  void SetMelody(int semis, uint8_t vel, bool main, bool tie = false) {
    if (main) {
      // the note after a tied one slides into place (303 style), in half a semicorchea
      if (slide_next) { glide_from = MelodyCV(); glide_len = glide_left = constrain(tick_period / 4, 17, 1700); }
      else glide_left = 0;
      slide_next = tie;
    }
    if (main || p[ORN_CV]) melody_out = semis;
    if (!p[MIDI]) return;
    const int note = constrain(64 + semis + 12 * (p[OCT_B] - 4), 0, 127);
    if (mel_note >= 0) usbMIDI.sendNoteOff(mel_note, 0, 1);
    usbMIDI.sendNoteOn(note, vel, 1);
    mel_note = (int8_t)note;
  }
  void ChordHit(uint8_t vel, bool strum) {
    if (!p[MIDI]) return;
    for (int i = 0; i < 4; ++i) if (chord_on[i] >= 0) { usbMIDI.sendNoteOff(chord_on[i], 0, 2); chord_on[i] = -1; }
    int8_t iv[4]; const int n = engine.ChordTones(iv);
    const int root = 52 + ((engine.RootSemis() % 12) + 12) % 12;
    for (int i = 0; i < n; ++i) {
      int note = root + iv[i];
      if (p[VOICING] == 1 && (i == 1 || i == 3)) note += 12;
      chord_on[i] = (int8_t)note;
      if (!strum || i == 0) usbMIDI.sendNoteOn(note, vel, 2);
      else Enqueue(STRUM_STEP * i, Q_CNOTE, note, vel - 8 * i);
    }
  }
  void AllNotesOff() {
    if (mel_note >= 0) usbMIDI.sendNoteOff(mel_note, 0, 1);
    if (bass_note >= 0) usbMIDI.sendNoteOff(bass_note, 0, 3);
    if (carp_note >= 0) usbMIDI.sendNoteOff(carp_note, 0, 2);
    mel_note = bass_note = carp_note = -1;
    for (int i = 0; i < 4; ++i) if (chord_on[i] >= 0) { usbMIDI.sendNoteOff(chord_on[i], 0, 2); chord_on[i] = -1; }
  }

  // ---------------------------------------------------------------- presets
  void Compact(CompactMat &c) const {
    compas::Engine::Material m; engine.GetMaterial(m);
    c.r0 = m.r_seed[0]; c.m0 = m.m_seed[0]; c.h = m.h_seed; c.mp0 = m.m_pos[0];
    c.pos = m.pos; c.valid = (m.r_valid & 1) | ((m.m_valid & 1) << 1) | (m.h_valid ? 4 : 0);
  }
  void Uncompact(const CompactMat &c) {
    compas::Engine::Material m;
    for (int i = 0; i < compas::MAX_LOOP; ++i) { m.r_seed[i] = c.r0; m.m_seed[i] = c.m0; m.m_pos[i] = c.mp0; }
    m.h_seed = c.h; m.entropy = c.r0 ^ c.m0 ^ c.h ^ 0x9E37u; m.pos = c.pos;
    m.r_valid = c.valid & 1; m.m_valid = (c.valid >> 1) & 1; m.h_valid = (c.valid >> 2) & 1;
    PushParams();
    engine.SetMaterial(m);
  }
  void PushParams() {
    engine.style = WrapStyle(p[STYLE]); engine.fam = p[FAM]; engine.prog = p[PROG];
    engine.loop_r = p[LOOP_R]; engine.loop_m = p[LOOP_M]; engine.loop_h = p[LOOP_H];
  }
  void SavePreset(int slot) {
    FlashPreset fp;
    engine.GetMaterial(fp.mat);
    for (int i = 0; i < NUM_PRESET_PARAMS; ++i) fp.v[i] = (int8_t)p[i];
    if (slot < 8) {
      for (int i = 0; i < NUM_PRESET_PARAMS; ++i) ee[slot].v[i] = fp.v[i];
      Compact(ee[slot].mat);
    }
    const bool ok = xuzu::SavePreset(xuzu::D_COMPAS, slot, 1, &fp, sizeof(fp));
    // the app state is saved too, so the module boots like this
    OC::save_app_data();
    Message(ok ? "Guardado" : "Error flash");
  }
  bool HasPreset(int slot) const {
    return xuzu::PresetValid(xuzu::D_COMPAS, slot, 1) || (slot < 8 && ee[slot].v[0] >= 0);
  }
  void RequestLoad(int slot, bool with_material) {
    if (!HasPreset(slot)) { Message("Vacio"); return; }
    pending_load = slot; pending_with_material = with_material;
    p[PRESET] = slot;
    if (!with_material) ApplyPendingLoad();        // settings only: right now, keeps what is playing
  }
  void ApplyPendingLoad() {
    const int slot = pending_load; pending_load = -1;
    FlashPreset fp;
    const bool in_flash = xuzu::LoadPreset(xuzu::D_COMPAS, slot, 1, &fp, sizeof(fp));
    const int8_t *v = in_flash ? fp.v : ee[slot].v;
    if (!in_flash && !(slot < 8 && ee[slot].v[0] >= 0)) return;
    for (int i = 0; i < NUM_PRESET_PARAMS; ++i) p[i] = constrain(v[i], MIN_[i], MAX_[i]);
    if (pending_with_material) {
      if (in_flash) { PushParams(); engine.SetMaterial(fp.mat); }
      else Uncompact(ee[slot].mat);
      layer[0].frozen = layer[1].frozen = false;
      layer[0].count = layer[1].count = 0;
      Message("Cargado");
    } else {
      Message("Ajustes cargados");
    }
  }
  void Message(const char *m) { msg = m; msg_timer = 16667 * 2; }

  // ---------------------------------------------------------------- editing
  void Edit(int which, int dir) {
    if (which == DESPL) {               // moves everything one Pam clock earlier (+) or later (-)
      p[DESPL] = constrain(p[DESPL] + dir, -99, 99);
      if (dir > 0) extra_clocks += dir; else skip_clocks += -dir;
      return;
    }
    if (which == LOAD_ALL || which == LOAD_SET) { if (dir > 0) RequestLoad(p[PRESET], which == LOAD_ALL); return; }
    if (which == SAVE) { if (dir > 0) SavePreset(p[PRESET]); return; }
    int mx = MAX_[which];
    if (which == PROG) mx = compas::Engine::NumProgs(p[FAM]) - 1;
    p[which] = constrain(p[which] + dir * STEP_[which], MIN_[which], mx);
    if (which == FAM) p[PROG] = constrain(p[PROG], 0, compas::Engine::NumProgs(p[FAM]) - 1);
    if (which == MIDI || which == CHORDS || which == BASS) AllNotesOff();
    if (which == MCLK && !p[MCLK]) started = false;
  }
  void PrintValue(int which) {
    static const char *const LOCKS[compas::NUM_LOCKS] = {"Nada", "Ritmo", "Melodia", "Armonia", "Ritmo+mel", "Todo"};
    static const char *const REH[4] = {"No", "Septimas", "Dominantes", "Sustitucion"};
    static const char *const OUTA[3] = {"Acentos", "Bombo 4x4", "Ambos"};
    static const char *const OUTB[3] = {"Melodia", "Bajo", "Arpegio"};
    static const char *const OUTD[4] = {"Raiz", "Just Friends", "Gate melodia", "Acento (acid)"};
    static const char *const BMODE[6] = {"No", "Remate", "Acentos", "Cada tiempo", "Cada ciclo", "Azar"};
    static const char *const BLEN[4] = {"1/2 tiempo", "1 tiempo", "2 tiempos", "4 tiempos"};
    static const char *const CURVE[4] = {"Plana", "Sube", "Baja", "Campana"};
    static const char *const BOUT[3] = {"Salida C", "MIDI", "Ambos"};
    static const char *const CHORD[3] = {"Con ritmo", "Sostenidos", "Arpegio"};
    static const char *const BASSM[5] = {"No", "Raiz", "Octavas", "Contratiempo", "Arpegio"};
    static const char *const VELS[8] = {"/2", "x2/3", "x3/4", "x1", "x1.5", "x2", "x3", "x4"};
    static const char *const CVD[NUM_CV_DEST] = {"Nada", "Estilo", "Armonia", "Progresion", "Densidad", "Soltura",
      "Intensidad", "Loop congela", "Vel. loop", "Loop ritmo", "Loop melodia", "Adorno", "Notas", "Rearmonizar", "Rafaga", "Preset"};
    switch (which) {
    case STYLE: gfxPrint(compas::STYLES[p[STYLE]].name); break;
    case PERFIL: gfxPrint(p[PERFIL] ? "Techno" : "Tradicional"); break;
    case REMATES: case SEGUIR: case ORN_CV: case MIDI: case MCLK: gfxPrint(p[which] ? "Si" : "No"); break;
    case OUT_A: gfxPrint(OUTA[p[OUT_A]]); break;
    case FAM: gfxPrint(compas::FAMS[p[FAM]].name); break;
    case PROG: gfxPrint(compas::Engine::ProgName(p[FAM], p[PROG])); break;
    case REHARM: gfxPrint(REH[p[REHARM]]); break;
    case FRASE: gfxPrint(p[FRASE] ? "8 acordes" : "4 acordes"); break;
    case LOOP_R: case LOOP_M: gfxPrint(compas::LOOP_NAMES[p[which]]); break;
    case LOOP_H: gfxPrint(compas::HLOOP_NAMES[p[LOOP_H]]); break;
    case LOCK: gfxPrint(LOCKS[p[LOCK]]); break;
    case VEL_R: case VEL_M: gfxPrint(VELS[p[which]]); break;
    case MEL_STYLE: gfxPrint(p[MEL_STYLE] ? "Ornamentada" : "Clasica"); break;
    case OCT_B: case OCT_D: if (p[which] > 4) gfxPrint("+"); gfxPrint(p[which] - 4); break;
    case OUT_B: gfxPrint(OUTB[p[OUT_B]]); break;
    case OUT_D: gfxPrint(OUTD[p[OUT_D]]); break;
    case B_MODE: gfxPrint(BMODE[p[B_MODE]]); break;
    case B_LEN: gfxPrint(BLEN[p[B_LEN]]); break;
    case B_CURVE: gfxPrint(CURVE[p[B_CURVE]]); break;
    case B_OUT: gfxPrint(BOUT[p[B_OUT]]); break;
    case VOICING: gfxPrint(p[VOICING] ? "Abierto" : "Cerrado"); break;
    case CHORDS: gfxPrint(CHORD[p[CHORDS]]); break;
    case BASS: gfxPrint(BASSM[p[BASS]]); break;
    case CLK: gfxPrint(p[CLK] ? "x2 del Pam" : "x4 del Pam"); break;
    case CV1D: case CV2D: case CV3D: gfxPrint(CVD[p[which]]); break;
    case RESET_MODE: gfxPrint(p[RESET_MODE] ? "Rejilla" : "Ciclo"); break;
    case DESPL: if (p[DESPL] > 0) gfxPrint("+"); gfxPrint(p[DESPL]); break;
    case PRESET: gfxPrint(p[PRESET] + 1); gfxPrint(HasPreset(p[PRESET]) ? "" : " (vacio)"); break;
    case LOAD_ALL: case LOAD_SET: case SAVE: gfxPrint("gira >"); break;
    default: gfxPrint(p[which]); break;
    }
  }

  static int WrapStyle(int x) { return ((x % compas::NUM_STYLES) + compas::NUM_STYLES) % compas::NUM_STYLES; }
  static int Scale(int cv, int max) { return cv <= 0 ? 0 : cv * max / HSAPPLICATION_5V; }
  static int Sticky(int cv, int n, int &last) {
    if (cv < 0) cv = 0;
    const int v = cv * n * 100 / HSAPPLICATION_5V;
    if (v < last * 100 - 15 || v > (last + 1) * 100 + 15) last = constrain(v / 100, 0, n - 1);
    if (last >= n) last = n - 1;
    if (last < 0) last = 0;
    return last;
  }
  static const char *NoteName(int semis) { return compas::NOTE_NAMES[((semis % 12) + 12) % 12]; }

  void DrawClock() {
    const int n = engine.NumBeats();
    const int cx = 32, cy = 38, r = 21;
    const int cur = engine.Beat();
    for (int b = 0; b < n; ++b) {
      const int count = engine.CountAt(b);
      const float a = (float)(count % n) / n * 6.2831853f - 1.5707963f;
      const int x = cx + (int)(r * cosf(a)), y = cy + (int)(r * sinf(a));
      if (engine.tick >= 0 && b == cur) gfxRect(x - 2, y - 2, 5, 5);
      else if (engine.BeatHasA(b)) gfxFrame(x - 2, y - 2, 5, 5);
      else gfxPixel(x, y);
    }
    gfxPrint(cx - 5, cy - 3, engine.tick < 0 ? 0 : engine.CountAt(cur));
  }
};

constexpr const char *Compas::PAGE_NAMES[];
constexpr int Compas::PAGE_LEN[];
constexpr uint8_t Compas::PAGE_PARAMS[][8];
constexpr const char *Compas::NAMES[];
constexpr int Compas::MIN_[];
constexpr int Compas::MAX_[];
constexpr int Compas::STEP_[];
constexpr uint8_t Compas::VOICE_NOTE[];
constexpr uint8_t Compas::VEL_NUM[];
constexpr uint8_t Compas::VEL_DEN[];

Compas Compas_instance;

void Compas_init() { Compas_instance.BaseStart(); }
static constexpr size_t Compas_storageSize() { return sizeof(Compas::State); }
size_t Compas_save(void *dst) { return Compas_instance.SaveState(dst); }
size_t Compas_restore(const void *src) { return Compas_instance.RestoreState(src); }
void Compas_isr() { Compas_instance.BaseController(); }
void Compas_handleAppEvent(OC::AppEvent event) {
  if (event == OC::APP_EVENT_RESUME) Compas_instance.Resume();
  if (event == OC::APP_EVENT_SUSPEND) Compas_instance.Suspend();
}
void Compas_loop() {}
void Compas_menu() { Compas_instance.BaseView(); }
void Compas_screensaver() { xuzu::fx::Draw(); }
void Compas_handleButtonEvent(const UI::Event &event) {
  if (event.type == UI::EVENT_BUTTON_PRESS) Compas_instance.OnButton(event.control);
  if (event.type == UI::EVENT_BUTTON_LONG_PRESS) Compas_instance.OnLongPress(event.control);
}
void Compas_handleEncoderEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_ENCODER_L) Compas_instance.OnLeftEncoderMove(event.value);
  if (event.control == OC::CONTROL_ENCODER_R) Compas_instance.OnRightEncoderMove(event.value);
}

#endif // ENABLE_APP_COMPAS
