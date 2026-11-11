// Brains Seq engine (xuzu rebuild of v4.16): a preset sequencer for Brains (a Plaits clone).
// 4 slots. Each slot listens to a source (Euclid x4 track, or the internal Compas) and holds a
// preset for the Brains: model voltage (MODEL input) and two CVs (value or the source's note).
// Every hit of a slot's source: B = model, C / D = slot values, then the trigger on A.
// Shared by the Brains Seq app and the Brains applet.
#pragma once
#include <Arduino.h>
#include "xuzu/compas_engine.h"
#include "xuzu/euclid4_engine.h"

namespace brains {

enum Source : uint8_t { S_OFF = 0, S_E1, S_E2, S_E3, S_E4, S_C_ACC, S_C_FILL, S_C_BASS, S_C_MEL, NUM_SOURCES };
static const char *const SOURCE_NAMES[NUM_SOURCES] = {"Apagado", "Euclid T1", "Euclid T2", "Euclid T3", "Euclid T4",
  "Compas acentos", "Compas relleno", "Compas bajo", "Compas melodia"};

struct __attribute__((packed)) Slot {
  uint8_t source, model, c_note, c_val, d_note, d_val; int8_t oct;   // values 0..100 = 0..5 V
};
enum MacroDest { MD_HITS = 0, MD_PROB, MD_AZAR, MD_ROT, MD_DENS, MD_SOLT, MD_SLOTS, NUM_MACRO };

struct __attribute__((packed)) Settings {
  Slot slot[4];
  uint8_t c_style, c_fam, c_dens, c_solt;       // internal Compas
  int8_t macro_base; int8_t macro_amt[NUM_MACRO];
  uint8_t loop_len, priority, transition, trig_delay;   // loop 0/1/2 = 1/2/4 bars, priority 0 low 1 high
  uint8_t euclid_preset;                        // 0 = no change, 1..16
};

class Engine {
public:
  Settings s;
  uint8_t calib[8];             // model voltages in 1/20 V (0.05 V)
  uint8_t clk_x2 = 0;
  bool loop_fixed = false;

  // outputs
  int out_model = 0, out_c = 0, out_d = 0;      // DAC values (128 per semitone, 1536 per volt)
  bool trig = false;                            // a trigger this ISR tick
  int last_slot = -1;
  bool looping = false;

  compas::Engine compas;

  void Init() {
    static const Slot DEF[4] = {{S_E1, 0, 0, 50, 0, 50, 0}, {S_E2, 1, 0, 50, 0, 50, 0}, {S_C_BASS, 2, 1, 50, 0, 50, -1}, {S_C_MEL, 3, 1, 50, 0, 50, 0}};
    for (int i = 0; i < 4; ++i) s.slot[i] = DEF[i];
    s.c_style = compas::S_BULERIA; s.c_fam = compas::H_AUTO; s.c_dens = 55; s.c_solt = 15;
    s.macro_base = 0; for (int i = 0; i < NUM_MACRO; ++i) s.macro_amt[i] = 0;
    s.loop_len = 0; s.priority = 0; s.transition = 0; s.trig_delay = 1; s.euclid_preset = 0;
    for (int i = 0; i < 8; ++i) calib[i] = (uint8_t)(i * 12);      // 0.6 V per model (a guess: tune by ear)
    compas.Reset(); compas.Seed(0xB2A1u ^ micros());
    compas.perfil = 1; compas.loop_r = 4; compas.loop_m = 4;
  }

  static int Volts(int v100) { return v100 * 1536 * 5 / 100; }       // 0..100 -> 0..5 V
  static int Calib(int u) { return u * 1536 / 20; }

  // ---- inputs, every ISR tick
  void SetMacro(int cv) { macro_cv = cv; }                // DAC units, 7680 = 5 V
  void SetTransition(int cv) { trans_cv = cv; }
  void SetLoopGate(bool g) { loop_gate = g; }

  // clock from TR1 (x4 of the Pam: one pulse per semicorchea)
  void Clock(int period) {
    const int p16 = clk_x2 ? period / 2 : period;
    ApplyMacro();
    // Euclid x4 tracks, as programmed in the app
    euclid_engine.Clock16(p16);
    if (clk_x2) pending16 = p16 > 0 ? p16 : 1;
    // the internal Compas: one corchea per semicorchea in flamenco styles, every two otherwise
    CompasClock(p16);
    // loop bookkeeping: one slot per semicorchea
    LoopAdvance();
  }
  void Reset() {
    euclid_engine.Reset(); compas.Realign(); c_count = 0; pos16 = 0;
  }
  void NewMaterial() { compas.Reroll(); }

  void Update() {
    trig = false;
    if (pending16 > 0 && --pending16 == 0) { euclid_engine.Clock16(p16_last); CompasClock(p16_last); LoopAdvance(); }
    euclid_engine.Update();
    // compas queue (fine steps inside the corchea)
    for (int i = 0; i < CQ; ++i) {
      if (cq_time[i] < 0 || --cq_time[i] > 0) continue;
      cq_time[i] = -1;
      Candidate(cq_src[i], cq_note[i]);
    }
    // Euclid hits
    for (int k = 0; k < 4; ++k) if (euclid_engine.hit[k]) Candidate(S_E1 + k, euclid_engine.pitch[k] - 48);
    ResolveCandidates();
    euclid_engine.EndTick();

    // delayed trigger: the model changes first, then the Brains is struck
    if (trig_wait > 0 && --trig_wait == 0) trig = true;
    // C / D transition
    const int tt = TransitionTicks();
    Glide(out_c, target_c, tt, c_from, glide_t);
    Glide(out_d, target_d, tt, d_from, glide_t);
    if (glide_t < tt) glide_t++;
  }

  int ActiveSlots() const { return active_slots; }

private:
  int macro_cv = 0, trans_cv = 0; bool loop_gate = false;
  int pending16 = 0, p16_last = 1000;
  uint32_t c_count = 0;
  int pos16 = 0;
  int active_slots = 4;
  int trig_wait = 0;
  int target_c = 0, target_d = 0, c_from = 0, d_from = 0, glide_t = 0;
  // candidates collected during one ISR tick (simultaneous hits)
  int cand_mask = 0; int cand_note[4] = {0, 0, 0, 0};

  static constexpr int CQ = 24;
  int cq_time[CQ]; uint8_t cq_src[CQ]; int8_t cq_note[CQ];

  // ---- loop memory: what each semicorchea played, for up to 4 bars
  static constexpr int LMAX = 64;
  struct LStep { int8_t slot; int8_t note; };
  LStep hist[LMAX]; int hist_w = 0;
  LStep loop[LMAX]; int loop_n = 0, loop_i = 0;
  bool arm = false;
  LStep cur = {-1, 0};

public:
  Engine() { for (int i = 0; i < CQ; ++i) cq_time[i] = -1; for (int i = 0; i < LMAX; ++i) hist[i] = loop[i] = {-1, 0}; }
private:

  void CompasClock(int p16) {
    p16_last = p16;
    compas.style = s.c_style; compas.fam = s.c_fam;
    const bool flam = compas.Def().flags & compas::F_CELLS;
    if (!flam && (c_count++ & 1)) return;
    const int tp = flam ? p16 : p16 * 2;
    const compas::Event ev = compas.Tick();
    for (int k = 0; k < compas::FINE; ++k) {
      const int d = tp * k / compas::FINE;
      if (ev.fine_a & (1u << k)) { Enq(d, S_C_ACC, 0); Enq(d, S_C_BASS, (int8_t)(compas.RootSemis() % 12 - 12)); }
      if (ev.fine_b & (1u << k)) Enq(d, S_C_FILL, 0);
    }
    if (ev.mel_n) Enq(tp * ev.mel_at[ev.mel_main] / compas::FINE, S_C_MEL, (int8_t)(ev.mel_semis[ev.mel_main] - 12));
  }
  void Enq(int d, uint8_t src, int8_t note) {
    if (d <= 0) { Candidate(src, note); return; }
    for (int i = 0; i < CQ; ++i) if (cq_time[i] < 0) { cq_time[i] = d; cq_src[i] = src; cq_note[i] = note; return; }
  }

  void Candidate(int src, int note) {
    if (looping) return;                  // the loop replays its own hits
    for (int i = 0; i < 4; ++i) {
      if (s.slot[i].source != src || !SlotActive(i)) continue;
      cand_mask |= 1 << i; cand_note[i] = note;
    }
  }
  bool SlotActive(int i) const { return i < active_slots; }

  void ResolveCandidates() {
    if (!cand_mask) return;
    int pick = -1;
    if (s.priority == 0) { for (int i = 0; i < 4; ++i) if (cand_mask & (1 << i)) { pick = i; break; } }
    else { for (int i = 3; i >= 0; --i) if (cand_mask & (1 << i)) { pick = i; break; } }
    const int note = cand_note[pick];
    cand_mask = 0;
    Play(pick, note);
    cur = {(int8_t)pick, (int8_t)note};
  }

  void Play(int i, int note) {
    const Slot &sl = s.slot[i];
    last_slot = i;
    out_model = Calib(calib[sl.model & 7]);
    c_from = out_c; d_from = out_d; glide_t = 0;
    const int n = (note + 12 * sl.oct) * 128;
    target_c = sl.c_note ? n : Volts(sl.c_val);
    target_d = sl.d_note ? n : Volts(sl.d_val);
    if (TransitionTicks() == 0) { out_c = target_c; out_d = target_d; }
    trig_wait = s.trig_delay ? s.trig_delay * 17 : 1;
  }

  int TransitionTicks() const {
    // menu base + CV2: 0 V = jump, 5 V = 2 s
    int ms = s.transition * 20 + (trans_cv > 0 ? trans_cv * 2000 / 7680 : 0);
    return ms > 0 ? ms * 17 : 0;
  }
  static void Glide(int &out, int target, int tt, int from, int t) {
    if (tt <= 0 || t >= tt) { out = target; return; }
    out = from + (target - from) * t / tt;
  }

  void ApplyMacro() {
    const int m = constrain(s.macro_base + macro_cv * 100 / 7680, -100, 100);   // -100..100
    for (int k = 0; k < 4; ++k) {
      euclid_engine.cv_hits[k] = s.macro_amt[MD_HITS] * m * 16 / 10000;
      euclid_engine.cv_prob[k] = s.macro_amt[MD_PROB] * m / 100;
      euclid_engine.cv_azar[k] = s.macro_amt[MD_AZAR] * m / 100;
      euclid_engine.cv_rot[k] = s.macro_amt[MD_ROT] * m * 16 / 10000;
    }
    compas.density = constrain(s.c_dens + s.macro_amt[MD_DENS] * m / 100, 0, 100);
    compas.soltura = constrain(s.c_solt + s.macro_amt[MD_SOLT] * m / 100, 0, 100);
    // slots: positive amount -> more macro leaves fewer slots playing; negative -> the reverse
    const int a = s.macro_amt[MD_SLOTS], mm = m > 0 ? m : 0;
    if (a > 0) active_slots = 4 - (3 * a * mm + 5000) / 10000;
    else if (a < 0) active_slots = 1 + (3 * -a * mm + 5000) / 10000;
    else active_slots = 4;
    active_slots = constrain(active_slots, 1, 4);
  }

  // one semicorchea of loop memory
  void LoopAdvance() {
    const int bars = 1 << s.loop_len;
    const int L = 16 * bars;
    const bool want = loop_gate || loop_fixed;
    hist[hist_w] = cur; cur = {-1, 0};
    hist_w = (hist_w + 1) % LMAX;
    pos16 = (pos16 + 1) % 16;
    if (want && !looping && !arm) arm = true;
    if (!want) { arm = false; looping = false; }
    if (arm && pos16 == 0) {                    // at the next 1: grab the last bars
      for (int i = 0; i < L; ++i) loop[i] = hist[(hist_w - L + i + LMAX * 2) % LMAX];
      loop_n = L; loop_i = 0; looping = true; arm = false;
    }
    if (looping) {
      const LStep &st = loop[loop_i];
      loop_i = (loop_i + 1) % loop_n;
      if (st.slot >= 0) { Play(st.slot, st.note); cur = st; }
    }
  }
};

} // namespace brains

extern brains::Engine brains_engine;
