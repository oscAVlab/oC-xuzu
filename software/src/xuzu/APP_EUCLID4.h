// Euclid x4 (xuzu rebuild of v4.16): 4-track Euclidean sequencer for techno.
// T1 kick, T2 percussion, T3 lead or bass, T4 extra voice.
//
// TR1 clock (x4 / x2 of the Pam, or internal)   TR2-TR4 assignable (Reset, Fill, Mute T1 by default)
// CV1-CV3 assignable with a track each          CV4 preset 1-16
// Outputs, patch "T1 V1 T2 V2": A trigger V1, B pitch V1, C trigger V2, D pitch V2 (V1 = T1, V2 = T3)
// MIDI: T1 ch 11, T2 ch 12, T3 ch 13, T4 ch 14
// Right encoder: turn = track, press = mute.  Long left: new seed for the track.  Long DOWN: one-bar fill.

#ifdef ENABLE_APP_EUCLID4

#include "HSApplication.h"
#include "xuzu/euclid4_engine.h"
#include "xuzu/xuzu_flash.h"

euclid4::Engine euclid_engine;

class Euclid4 : public HSApplication {
public:
  enum Param {
    // per track (edited on the selected track)
    LEN, STEPS, HITS, ROT, RATIO,
    PROB, AZAR, SEMILLA, MUTE, SWING,
    BURST, BHITS, BROT, BMODE, BDIV,
    TIPO, MEL, TONO, OCTV, GATE, SCALE, ROOT,
    MNOTE, MCHORD, MCHAN, MIDI, MCLK,
    PATCH, VOZ1, VOZ2, OUTA, OUTB, OUTC, OUTD,
    CV1D, CV1T, CV2D, CV2T, CV3D, CV3T,
    CLK, BPM, DESPL, TR2, TR3, TR4,
    PRESET, LOADP, SAVEP, PATRON, MORPH, GOPAT, SAVEM, SAVESTATE,
    NUM_PARAMS };
  enum TrDest { TR_NADA, TR_RESET, TR_FILL, TR_NEXT, TR_MUTE1, TR_MUTE2, TR_MUTE3, TR_MUTE4, TR_SEEDS, NUM_TR };
  enum CvDest { CV_NADA, CV_HITS, CV_ROT, CV_STEPS, CV_LEN, CV_PROB, CV_AZAR, CV_RATIO, CV_BURST, CV_TONO, CV_MUTE, CV_FILL, CV_PATRON, NUM_CVD };
  enum OutSrc { O_T1, O_T2, O_T3, O_T4, O_P1, O_P2, O_P3, O_P4, O_CLK16, O_RESET, NUM_OUTSRC };

  struct __attribute__((packed)) Rack {
    uint8_t patch, voz1, voz2, out[4], cvd[3], cvt[3], clk, bpm, tr[3], midi, mclk, pattern, morph, preset;
  };
  struct __attribute__((packed)) State { euclid4::Track t[euclid4::NT]; uint8_t swing, scale, root; Rack rack; };
  struct FlashPreset { xuzu::PresetHeader h; euclid4::Track t[euclid4::NT]; uint8_t swing, scale, root; };
  static_assert(sizeof(FlashPreset) <= xuzu::PRESET_SIZE, "Euclid preset too big");
  static_assert(sizeof(State) != 69, "must differ from the v4.16 layout");

  void Start() {
    euclid_engine.Init();
    rack = Rack{0, 0, 2, {O_T1, O_P1, O_T3, O_P3}, {CV_HITS, CV_ROT, CV_TONO}, {1, 2, 2}, 0, 125, {TR_RESET, TR_FILL, TR_MUTE1}, 1, 0, 0, 0, 0};
    for (int k = 0; k < 4; ++k) mem.t[k] = euclid_engine.t[k];
    mem.swing = 0;
  }
  void Resume() {}
  void Suspend() {}

  euclid4::Engine &E() { return euclid_engine; }

  // ------------------------------------------------------------------ ISR
  void Controller() {
    euclid4::Engine &e = E();
    e.midi = rack.midi; e.mclk = rack.mclk;
    ApplyCV();

    // TR2-TR4
    for (int i = 0; i < 3; ++i) {
      const int ch = i + 1;
      const bool g = frame.gate_high[ch];
      switch (rack.tr[i]) {
        case TR_RESET: if (frame.clocked[ch]) { e.Reset(); since_clock = 0; } break;
        case TR_FILL: if (g) fill_gate = true; break;
        case TR_NEXT: if (frame.clocked[ch]) NextPattern(); break;
        case TR_MUTE1: case TR_MUTE2: case TR_MUTE3: case TR_MUTE4: tr_mute[rack.tr[i] - TR_MUTE1] |= g; break;
        case TR_SEEDS: if (frame.clocked[ch]) for (int k = 0; k < 4; ++k) e.NewSeed(k); break;
        default: break;
      }
    }

    // clock
    if (since_clock < 2000000) since_clock++;
    bool clk = false; int per = since_clock;
    if (rack.clk == 2) {                       // internal
      const int per16 = 16667 * 60 / (rack.bpm * 4);
      if (++int_timer >= per16) { int_timer = 0; clk = true; per = per16; }
    } else if (frame.clocked[0]) {
      clk = true;
      if (since_clock > 16667) { e.Reset(); per = last_period; }   // stopped for 1 s: this clock is the 1
      else last_period = since_clock;
      since_clock = 0;
    }
    if (clk) {
      if (skip > 0) skip--;
      else if (rack.clk == 1) { e.Clock16(per / 2); Enqueue16(per / 2); }   // x2: two semicorcheas per pulse
      else e.Clock16(per);
    }
    while (extra > 0) { extra--; e.Clock16(last_period); }
    if (pending16 > 0 && --pending16 == 0) e.Clock16(last_period / 2);

    // fill: gate input or one bar from DOWN
    if (fill_bar > 0 && e.clock16) fill_bar--;
    e.fill = fill_gate || fill_bar > 0 || cv_fill;
    for (int k = 0; k < 4; ++k) e.mute[k] = mutes[k] || tr_mute[k];

    e.Update();
    Outputs();
    if (e.cycle_reset) cycle_flash = 800;
    e.EndTick();
    fill_gate = false; for (int k = 0; k < 4; ++k) tr_mute[k] = false;
    for (int k = 0; k < 4; ++k) if (flash[k]) flash[k]--;
    if (cycle_flash) cycle_flash--;
    if (msg_timer) msg_timer--;
  }

  // ------------------------------------------------------------------ UI
  void View() {
    gfxHeader("Euclid x4");
    gfxPrint(74, 1, PAGE_NAMES[page]);
    // the four tracks: pattern + playhead
    for (int k = 0; k < 4; ++k) {
      const int y = 13 + k * 8;
      gfxPrint(0, y, k + 1);
      if (E().mute[k]) gfxPrint(6, y, "m");
      const int L = E().Len(k), n = L < 16 ? L : 16, pos = E().StepPos(k);
      const int start = (pos / 16) * 16;
      for (int i = 0; i < n; ++i) {
        const int si = start + i; if (si >= L) break;
        const int x = 14 + i * 3;
        if (E().PatternBit(k, si % E().Steps(k))) gfxRect(x, y + 1, 2, 5); else gfxPixel(x, y + 5);
      }
      const int pi = (pos + L - 1) % L - start;
      if (pi >= 0 && pi < 16) gfxInvert(14 + pi * 3 - 1, y, 4, 7);
      if (flash[k]) gfxRect(64, y + 1, 3, 5);
      if (k == track) gfxFrame(0, y - 1, 12, 9);
    }
    if (E().Morphing()) gfxPrint(70, 13, "Morph...");
    gfxLine(0, 45, 127, 45);
    const int prm = PAGE_PARAMS[page][cursor];
    gfxPrint(0, 47, PerTrack(prm) ? "T" : "");
    if (PerTrack(prm)) { gfxPrint(track + 1); gfxPrint(" "); }
    gfxPrint(NAMES[prm]);
    gfxPrint(0, 56, "");
    if (msg_timer && (prm == SAVEP || prm == LOADP || prm == SAVEM || prm == GOPAT || prm == SAVESTATE || prm == PRESET)) gfxPrint(msg);
    else PrintValue(prm);
    gfxInvert(0, 46, 128, 18);
  }

  void OnButton(int control) {
    switch (control) {
    case OC::CONTROL_BUTTON_UP:   page = (page + NUM_PAGES - 1) % NUM_PAGES; cursor = 0; break;
    case OC::CONTROL_BUTTON_DOWN: page = (page + 1) % NUM_PAGES; cursor = 0; break;
    case OC::CONTROL_BUTTON_L:    cursor = (cursor + 1) % PAGE_LEN[page]; break;
    case OC::CONTROL_BUTTON_R:    mutes[track] = !mutes[track]; break;
    default: break;
    }
  }
  void OnLongPress(int control) {
    if (control == OC::CONTROL_BUTTON_DOWN) fill_bar = 16;
    if (control == OC::CONTROL_BUTTON_L) { E().NewSeed(track); Message("Nueva semilla"); }
  }
  void OnLeftEncoderMove(int dir) { Edit(PAGE_PARAMS[page][cursor], dir); }
  void OnRightEncoderMove(int dir) { track = constrain(track + dir, 0, 3); }

  // ------------------------------------------------------------------ storage
  size_t SaveState(void *dst) {
    State *s = (State *)dst;
    for (int k = 0; k < 4; ++k) s->t[k] = E().t[k];
    s->swing = E().swing; s->scale = E().scale; s->root = E().root; s->rack = rack;
    return sizeof(State);
  }
  size_t RestoreState(const void *src) {
    const State *s = (const State *)src;
    if (s->t[0].len == 0 || s->t[0].len > euclid4::MAXLEN) return sizeof(State);
    for (int k = 0; k < 4; ++k) E().t[k] = s->t[k];
    E().swing = s->swing; E().scale = s->scale % euclid4::NUM_SCALES; E().root = s->root % 12; rack = s->rack;
    return sizeof(State);
  }

  // used by Brains Seq ("Euclid preset")
  bool LoadPresetSlot(int slot) {
    FlashPreset fp;
    if (!xuzu::LoadPreset(xuzu::D_EUCLID, slot, 1, &fp, sizeof(fp))) return false;
    euclid4::Pattern p;
    for (int k = 0; k < 4; ++k) p.t[k] = fp.t[k];
    p.swing = fp.swing;
    E().scale = fp.scale % euclid4::NUM_SCALES; E().root = fp.root % 12;
    E().MorphTo(p, MorphBars());
    return true;
  }

private:
  static constexpr int NUM_PAGES = 9;
  static constexpr const char *PAGE_NAMES[NUM_PAGES] = {"Euclides", "Azar", "Rafaga", "Nota", "MIDI", "Salidas", "Entradas CV", "Reloj / TR", "Patrones"};
  static constexpr int PAGE_LEN[NUM_PAGES] = {5, 5, 5, 7, 5, 7, 6, 6, 8};
  static constexpr uint8_t PAGE_PARAMS[NUM_PAGES][8] = {
    {LEN, STEPS, HITS, ROT, RATIO},
    {PROB, AZAR, SEMILLA, MUTE, SWING},
    {BURST, BHITS, BROT, BMODE, BDIV},
    {TIPO, MEL, TONO, OCTV, GATE, SCALE, ROOT},
    {MNOTE, MCHORD, MCHAN, MIDI, MCLK},
    {PATCH, VOZ1, VOZ2, OUTA, OUTB, OUTC, OUTD},
    {CV1D, CV1T, CV2D, CV2T, CV3D, CV3T},
    {CLK, BPM, DESPL, TR2, TR3, TR4},
    {PRESET, LOADP, SAVEP, PATRON, MORPH, GOPAT, SAVEM, SAVESTATE}};
  static constexpr const char *NAMES[NUM_PARAMS] = {
    "Largo", "Pasos", "Golpes", "Rotacion", "Ratio",
    "Probab.", "Azar", "Semilla", "Mute", "Swing",
    "Rafaga", "Golpes raf.", "Rot. raf.", "Modo raf.", "Division",
    "Tipo", "Melodia", "Tono", "Octava", "Gate", "Escala", "Raiz",
    "Nota MIDI", "Acorde MIDI", "Canal MIDI", "MIDI USB", "Clock MIDI",
    "Patch", "Voz 1", "Voz 2", "Salida A", "Salida B", "Salida C", "Salida D",
    "CV1", "CV1 pista", "CV2", "CV2 pista", "CV3", "CV3 pista",
    "Reloj", "BPM interno", "Desplazar", "TR2", "TR3", "TR4",
    "Preset", "Cargar preset", "Guardar preset", "Patron", "Morph", "Ir al patron", "Guardar en M", "Guardar estado"};
  static bool PerTrack(int p) { return p <= GATE || (p >= MNOTE && p <= MCHAN); }

  Rack rack;
  euclid4::Pattern mem;     // memory M (RAM only)
  bool mem_valid = false;
  int page = 0, cursor = 0, track = 0;
  int since_clock = 2000000, last_period = 2000, int_timer = 0, skip = 0, extra = 0, pending16 = 0, despl = 0;
  bool mutes[4] = {false, false, false, false}, tr_mute[4] = {false, false, false, false};
  bool fill_gate = false, cv_fill = false; int fill_bar = 0;
  int flash[4] = {0, 0, 0, 0}, cycle_flash = 0;
  int cv_last[3] = {0, 0, 0}, cv_preset = -1, cv_patron = -1;
  bool clock_out_state = false;
  const char *msg = ""; int msg_timer = 0;

  void Enqueue16(int d) { pending16 = d > 0 ? d : 1; }
  int MorphBars() const { static const uint8_t B[6] = {0, 1, 2, 4, 8, 16}; return B[rack.morph]; }

  void ApplyCV() {
    euclid4::Engine &e = E();
    e.ClearHM();      // the applets' modulation does not apply while the app runs
    for (int k = 0; k < 4; ++k) { e.cv_hits[k] = e.cv_rot[k] = e.cv_steps[k] = e.cv_len[k] = e.cv_prob[k] = e.cv_azar[k] = 0;
                                  e.cv_ratio[k] = e.cv_burst[k] = e.cv_pitch[k] = 0; e.cv_mute[k] = false; }
    cv_fill = false;
    for (int i = 0; i < 3; ++i) {
      const int cv = frame.inputs[i];
      const int d = rack.cvd[i];
      if (d == CV_NADA) continue;
      for (int k = 0; k < 4; ++k) {
        if (rack.cvt[i] != 4 && rack.cvt[i] != k) continue;   // 4 = Todas
        switch (d) {
          case CV_HITS:  e.cv_hits[k] += Bi(cv, 16); break;
          case CV_ROT:   e.cv_rot[k] += Bi(cv, 16); break;
          case CV_STEPS: e.cv_steps[k] += Bi(cv, 16); break;
          case CV_LEN:   e.cv_len[k] += Bi(cv, 16); break;
          case CV_PROB:  e.cv_prob[k] += Bi(cv, 100); break;
          case CV_AZAR:  e.cv_azar[k] += Bi(cv, 100); break;
          case CV_RATIO: e.cv_ratio[k] -= Bi(cv, 4); break;
          case CV_BURST: e.cv_burst[k] += Bi(cv, 7); break;
          case CV_TONO:  e.cv_pitch[k] += cv / 128; break;          // semitones of CV
          case CV_MUTE:  e.cv_mute[k] |= cv > HSAPPLICATION_3V / 2; break;
          default: break;
        }
      }
      if (d == CV_FILL) cv_fill |= cv > HSAPPLICATION_3V / 2;
      if (d == CV_PATRON) {
        const int v = constrain(cv * 9 / HSAPPLICATION_5V, 0, euclid4::NUM_FACTORY);
        if (v != cv_patron) { cv_patron = v; rack.pattern = v; GoPattern(); }
      }
    }
    const int s = xuzu::CVPreset(frame.inputs[3], HSAPPLICATION_5V, cv_preset);
    if (s >= 0) { rack.preset = s; LoadPreset(s); }
  }
  static int Bi(int cv, int range) { return cv * range / HSAPPLICATION_5V; }

  // ---- outputs
  void Outputs() {
    euclid4::Engine &e = E();
    for (int k = 0; k < 4; ++k) if (e.hit[k]) flash[k] = 600;
    int src[4];
    if (rack.patch == 0)      { src[0] = O_T1 + rack.voz1; src[1] = O_P1 + rack.voz1; src[2] = O_T1 + rack.voz2; src[3] = O_P1 + rack.voz2; }
    else if (rack.patch == 1) { src[0] = O_T1 + rack.voz1; src[1] = O_T1 + rack.voz2; src[2] = O_P1 + rack.voz1; src[3] = O_P1 + rack.voz2; }
    else for (int i = 0; i < 4; ++i) src[i] = rack.out[i];
    for (int ch = 0; ch < 4; ++ch) {
      const int s = src[ch];
      if (s <= O_T4) {
        const int k = s - O_T1;
        if (e.t[k].nota && e.gate_left[k] > 0) GateOut(ch, true);
        else if (e.hit[k]) ClockOut(ch, HEMISPHERE_CLOCK_TICKS * 5);
        else if (e.t[k].nota) GateOut(ch, false);
      } else if (s <= O_P4) {
        Out(ch, e.PitchCV(s - O_P1));
      } else if (s == O_CLK16) {
        if (e.clock16) ClockOut(ch, HEMISPHERE_CLOCK_TICKS * 5);
      } else if (s == O_RESET) {
        if (e.cycle_reset) ClockOut(ch, HEMISPHERE_CLOCK_TICKS * 5);
      }
    }
  }

  // ---- patterns and presets
  void GoPattern() {
    euclid4::Pattern p;
    if (rack.pattern < euclid4::NUM_FACTORY) euclid4::Factory(rack.pattern, p);
    else if (mem_valid) p = mem;
    else { Message("M vacia"); return; }
    E().MorphTo(p, MorphBars());
    Message(MorphBars() ? "Morph..." : "Cargado");
  }
  void NextPattern() { rack.pattern = (rack.pattern + 1) % (euclid4::NUM_FACTORY + (mem_valid ? 1 : 0)); GoPattern(); }
  void SavePreset(int slot) {
    FlashPreset fp;
    for (int k = 0; k < 4; ++k) fp.t[k] = E().t[k];
    fp.swing = E().swing; fp.scale = E().scale; fp.root = E().root;
    const bool ok = xuzu::SavePreset(xuzu::D_EUCLID, slot, 1, &fp, sizeof(fp));
    OC::save_app_data();
    Message(ok ? "Guardado" : "Error flash");
  }
  void LoadPreset(int slot) {
    // the outputs, CVs, clock and the mutes of the moment don't change
    Message(LoadPresetSlot(slot) ? "Cargado" : "Vacio");
  }
  void Message(const char *m) { msg = m; msg_timer = 16667 * 2; }

  // ---- editing
  uint8_t *TrackField(int prm) {
    euclid4::Track &t = E().t[track];
    switch (prm) {
      case LEN: return &t.len; case STEPS: return &t.steps; case HITS: return &t.hits; case ROT: return &t.rot; case RATIO: return &t.ratio;
      case PROB: return &t.prob; case AZAR: return &t.azar; case SEMILLA: return &t.seed_libre;
      case BURST: return &t.burst; case BHITS: return &t.bhits; case BROT: return &t.brot; case BMODE: return &t.bmode; case BDIV: return &t.bdiv;
      case TIPO: return &t.nota; case MEL: return &t.mel; case TONO: return &t.tono; case OCTV: return (uint8_t *)&t.oct; case GATE: return &t.gate;
      case MNOTE: return &t.midi_note; case MCHORD: return &t.chord; case MCHAN: return &t.chan;
      default: return nullptr;
    }
  }
  void Range(int prm, int &lo, int &hi, int &step) {
    lo = 0; step = 1;
    switch (prm) {
      case LEN: case STEPS: lo = 1; hi = euclid4::MAXLEN; break;
      case HITS: hi = E().t[track].steps; break;
      case ROT: hi = E().t[track].steps - 1; break;
      case RATIO: hi = 8; break;
      case PROB: case AZAR: case GATE: hi = 100; step = 5; break;
      case SEMILLA: case BMODE: case TIPO: case MIDI: case MCLK: hi = 1; break;
      case MUTE: hi = 1; break;
      case SWING: hi = 100; step = 5; break;
      case BURST: lo = 1; hi = 8; break;
      case BHITS: hi = 16; break; case BROT: hi = 15; break; case BDIV: hi = 8; break;
      case MEL: hi = 3; break; case TONO: hi = 127; break; case OCTV: lo = -3; hi = 3; break;
      case SCALE: hi = euclid4::NUM_SCALES - 1; break; case ROOT: hi = 11; break;
      case MNOTE: hi = 127; break; case MCHORD: hi = euclid4::NUM_CHORDS - 1; break; case MCHAN: lo = 1; hi = 16; break;
      case PATCH: hi = 2; break; case VOZ1: case VOZ2: hi = 3; break;
      case OUTA: case OUTB: case OUTC: case OUTD: hi = NUM_OUTSRC - 1; break;
      case CV1D: case CV2D: case CV3D: hi = NUM_CVD - 1; break;
      case CV1T: case CV2T: case CV3T: hi = 4; break;
      case CLK: hi = 2; break; case BPM: lo = 40; hi = 240; break;
      case TR2: case TR3: case TR4: hi = NUM_TR - 1; break;
      case PRESET: hi = xuzu::NUM_PRESETS - 1; break;
      case PATRON: hi = euclid4::NUM_FACTORY; break;
      case MORPH: hi = 5; break;
      default: hi = 1; break;
    }
  }
  int Get(int prm) {
    if (uint8_t *f = TrackField(prm)) return prm == OCTV ? (int)(int8_t)*f : *f;
    switch (prm) {
      case MUTE: return mutes[track]; case SWING: return E().swing; case SCALE: return E().scale; case ROOT: return E().root;
      case MIDI: return rack.midi; case MCLK: return rack.mclk; case PATCH: return rack.patch; case VOZ1: return rack.voz1; case VOZ2: return rack.voz2;
      case OUTA: case OUTB: case OUTC: case OUTD: return rack.out[prm - OUTA];
      case CV1D: return rack.cvd[0]; case CV2D: return rack.cvd[1]; case CV3D: return rack.cvd[2];
      case CV1T: return rack.cvt[0]; case CV2T: return rack.cvt[1]; case CV3T: return rack.cvt[2];
      case CLK: return rack.clk; case BPM: return rack.bpm; case DESPL: return despl;
      case TR2: case TR3: case TR4: return rack.tr[prm - TR2];
      case PRESET: return rack.preset; case PATRON: return rack.pattern; case MORPH: return rack.morph;
      default: return 0;
    }
  }
  void Set(int prm, int v) {
    if (uint8_t *f = TrackField(prm)) {
      *f = (uint8_t)v;
      euclid4::Track &t = E().t[track];
      if (t.hits > t.steps) t.hits = t.steps;
      if (t.rot >= t.steps) t.rot = t.steps - 1;
      return;
    }
    switch (prm) {
      case MUTE: mutes[track] = v; break; case SWING: E().swing = v; break; case SCALE: E().scale = v; break; case ROOT: E().root = v; break;
      case MIDI: rack.midi = v; break; case MCLK: rack.mclk = v; break; case PATCH: rack.patch = v; break;
      case VOZ1: rack.voz1 = v; break; case VOZ2: rack.voz2 = v; break;
      case OUTA: case OUTB: case OUTC: case OUTD: rack.out[prm - OUTA] = v; break;
      case CV1D: rack.cvd[0] = v; break; case CV2D: rack.cvd[1] = v; break; case CV3D: rack.cvd[2] = v; break;
      case CV1T: rack.cvt[0] = v; break; case CV2T: rack.cvt[1] = v; break; case CV3T: rack.cvt[2] = v; break;
      case CLK: rack.clk = v; break; case BPM: rack.bpm = v; break;
      case TR2: case TR3: case TR4: rack.tr[prm - TR2] = v; break;
      case PRESET: rack.preset = v; break; case PATRON: rack.pattern = v; break; case MORPH: rack.morph = v; break;
      default: break;
    }
  }
  void Edit(int prm, int dir) {
    switch (prm) {
      case DESPL: despl += dir; if (dir > 0) extra += dir; else skip += -dir; return;
      case LOADP: if (dir > 0) LoadPreset(rack.preset); return;
      case SAVEP: if (dir > 0) SavePreset(rack.preset); return;
      case GOPAT: if (dir > 0) GoPattern(); return;
      case SAVEM: if (dir > 0) { E().GetPattern(mem); mem_valid = true; Message("Guardado en M"); } return;
      case SAVESTATE: if (dir > 0) { OC::save_app_data(); Message("Guardado"); } return;
      default: break;
    }
    int lo, hi, st; Range(prm, lo, hi, st);
    Set(prm, constrain(Get(prm) + dir * st, lo, hi));
  }
  void PrintValue(int prm) {
    static const char *const TRN[NUM_TR] = {"Nada", "Reset", "Fill (gate)", "Sig. patron", "Mute T1", "Mute T2", "Mute T3", "Mute T4", "Semillas"};
    static const char *const CVN[NUM_CVD] = {"Nada", "Golpes", "Rotacion", "Pasos", "Largo", "Probab.", "Azar", "Ratio", "Rafaga",
                                             "Tono 1V/oct", "Mute (gate)", "Fill (gate)", "Patron"};
    static const char *const OSN[NUM_OUTSRC] = {"T1 trig", "T2 trig", "T3 trig", "T4 trig", "T1 tono", "T2 tono", "T3 tono", "T4 tono", "Reloj 1/16", "Reset ciclo"};
    static const char *const MORPHN[6] = {"Directo", "1 compas", "2 compases", "4 compases", "8 compases", "16 compases"};
    static const char *const CLKN[3] = {"x4 Pam", "x2 Pam", "Interno"};
    static const char *const PATCHN[3] = {"T1 V1 T2 V2", "T1 T2 V1 V2", "Libre"};
    const int v = Get(prm);
    switch (prm) {
      case LEN: case STEPS: gfxPrint(v); gfxPrint(" pasos"); break;
      case RATIO: gfxPrint(euclid4::RATIO_NAMES[v]); break;
      case PROB: case AZAR: case SWING: case GATE: gfxPrint(v); gfxPrint("%"); break;
      case SEMILLA: gfxPrint(v ? "Libre" : "Fija"); break;
      case MUTE: case MIDI: case MCLK: gfxPrint(v ? "Si" : "No"); break;
      case BURST: if (v <= 1) gfxPrint("No"); else gfxPrint(v); break;
      case BMODE: gfxPrint(v ? "Mordida" : "Subdividir"); break;
      case BDIV: gfxPrint(euclid4::BDIV_NAMES[v]); break;
      case TIPO: gfxPrint(v ? "Nota" : "Percusion"); break;
      case MEL: gfxPrint(euclid4::MEL_NAMES[v]); break;
      case OCTV: if (v > 0) gfxPrint("+"); gfxPrint(v); break;
      case SCALE: gfxPrint(euclid4::SCALE_NAMES[v]); break;
      case ROOT: gfxPrint(euclid4::NOTE_NAMES[v]); break;
      case MCHORD: gfxPrint(euclid4::CHORD_NAMES[v]); break;
      case PATCH: gfxPrint(PATCHN[v]); break;
      case VOZ1: case VOZ2: gfxPrint("Pista "); gfxPrint(v + 1); break;
      case OUTA: case OUTB: case OUTC: case OUTD: if (rack.patch != 2) gfxPrint("(patch) "); gfxPrint(OSN[v]); break;
      case CV1D: case CV2D: case CV3D: gfxPrint(CVN[v]); break;
      case CV1T: case CV2T: case CV3T: if (v == 4) gfxPrint("Todas"); else { gfxPrint("T"); gfxPrint(v + 1); } break;
      case CLK: gfxPrint(CLKN[v]); break;
      case TR2: case TR3: case TR4: gfxPrint(TRN[v]); break;
      case DESPL: if (v > 0) gfxPrint("+"); gfxPrint(v); break;
      case PRESET: gfxPrint(v + 1); gfxPrint(xuzu::PresetValid(xuzu::D_EUCLID, v, 1) ? "" : " (vacio)"); break;
      case PATRON: gfxPrint(euclid4::FACTORY_NAMES[v]); if (v == euclid4::NUM_FACTORY && !mem_valid) gfxPrint(" (vacia)"); break;
      case MORPH: gfxPrint(MORPHN[v]); break;
      case LOADP: case SAVEP: case GOPAT: case SAVEM: case SAVESTATE: gfxPrint("gira >"); break;
      default: gfxPrint(v); break;
    }
  }
};

constexpr const char *Euclid4::PAGE_NAMES[];
constexpr int Euclid4::PAGE_LEN[];
constexpr uint8_t Euclid4::PAGE_PARAMS[][8];
constexpr const char *Euclid4::NAMES[];

Euclid4 Euclid4_instance;

void Euclid4_init() { Euclid4_instance.BaseStart(); }
namespace xuzu { bool EuclidLoadPreset(int slot) { return Euclid4_instance.LoadPresetSlot(slot); } }
static constexpr size_t Euclid4_storageSize() { return sizeof(Euclid4::State); }
size_t Euclid4_save(void *dst) { return Euclid4_instance.SaveState(dst); }
size_t Euclid4_restore(const void *src) { return Euclid4_instance.RestoreState(src); }
void Euclid4_isr() { Euclid4_instance.BaseController(); }
void Euclid4_handleAppEvent(OC::AppEvent event) {
  if (event == OC::APP_EVENT_RESUME) Euclid4_instance.Resume();
  if (event == OC::APP_EVENT_SUSPEND) Euclid4_instance.Suspend();
}
void Euclid4_loop() {}
void Euclid4_menu() { Euclid4_instance.BaseView(); }
void Euclid4_screensaver() { Euclid4_instance.BaseScreensaver(true); }
void Euclid4_handleButtonEvent(const UI::Event &event) {
  if (event.type == UI::EVENT_BUTTON_PRESS) Euclid4_instance.OnButton(event.control);
  if (event.type == UI::EVENT_BUTTON_LONG_PRESS) Euclid4_instance.OnLongPress(event.control);
}
void Euclid4_handleEncoderEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_ENCODER_L) Euclid4_instance.OnLeftEncoderMove(event.value);
  if (event.control == OC::CONTROL_ENCODER_R) Euclid4_instance.OnRightEncoderMove(event.value);
}

#endif // ENABLE_APP_EUCLID4
