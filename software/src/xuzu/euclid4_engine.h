// Euclid x4 engine (xuzu rebuild of v4.16): the o_C version of the "Seq Euclidean x4" web sequencer.
// 4 tracks, each with its own cycle length, Euclidean pattern, ratio, probability / random,
// bursts and notes. Shared by the Euclid x4 app, the Eucl x4 applet and Brains Seq.
#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <string.h>

namespace euclid4 {

static constexpr int NT = 4;
static constexpr int MAXLEN = 64;

// ratios against the semicorchea (x5 .. /5), like the web app
static const uint8_t RATIO_N[9] = {5, 4, 3, 2, 1, 1, 1, 1, 1};
static const uint8_t RATIO_D[9] = {1, 1, 1, 1, 1, 2, 3, 4, 5};
static const char *const RATIO_NAMES[9] = {"x5", "x4", "x3", "x2", "x1", "/2", "/3", "/4", "/5"};

// burst subdivision against one beat (quarter); 0 = step / count
static const uint8_t BDIV[9] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
static const char *const BDIV_NAMES[9] = {"Paso", "1/2", "1/3", "1/4", "1/6", "1/8", "1/12", "1/16", "1/32"};

static constexpr int NUM_SCALES = 8;
static const int8_t SCALES[NUM_SCALES][12] = {
  {0, 2, 4, 5, 7, 9, 11, -1}, {0, 2, 3, 5, 7, 8, 10, -1}, {0, 2, 3, 5, 7, 9, 10, -1}, {0, 1, 3, 5, 7, 8, 10, -1},
  {0, 2, 4, 6, 7, 9, 11, -1}, {0, 2, 4, 5, 7, 9, 10, -1}, {0, 2, 4, 7, 9, -1}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
static const uint8_t SCALE_LEN[NUM_SCALES] = {7, 7, 7, 7, 7, 7, 5, 12};
static const char *const SCALE_NAMES[NUM_SCALES] = {"Mayor", "Menor", "Dorica", "Frigia", "Lidia", "Mixolidia", "Pentatonica", "Cromatica"};
static const char *const NOTE_NAMES[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

static constexpr int NUM_CHORDS = 7;
static const int8_t CHORDS[NUM_CHORDS][4] = {{0, -1, -1, -1}, {0, 7, -1, -1}, {0, 7, 12, -1}, {0, 3, 7, -1}, {0, 4, 7, -1}, {0, 3, 7, 10}, {0, 5, 7, -1}};
static const char *const CHORD_NAMES[NUM_CHORDS] = {"Una nota", "Quinta", "5a+8a", "Menor", "Mayor", "m7", "Sus4"};

enum MelMode : uint8_t { M_CURSOR = 0, M_AZAR, M_ESCALA, M_FRASE };
static const char *const MEL_NAMES[4] = {"Cursor", "Azar", "Escala", "Frase (loop)"};

// ---------------------------------------------------------------- one track
struct __attribute__((packed)) Track {
  uint8_t len = 16, steps = 16, hits = 4, rot = 0, ratio = 4;
  uint8_t prob = 100, azar = 0, seed_libre = 0;
  uint8_t burst = 1, bhits = 0, brot = 0, bmode = 0, bdiv = 0;   // bmode 0 Subdividir, 1 Mordida
  uint8_t nota = 0, mel = M_CURSOR, tono = 64; int8_t oct = 0; uint8_t gate = 50;
  uint8_t midi_note = 36, chord = 0, chan = 11;
  uint32_t seed = 1;
};
static constexpr int TRACK_NUMERIC = 13;   // fields that morph (len .. bdiv)

// ---------------------------------------------------------------- Euclidean pattern
// Bjorklund, same algorithm and rotation as the web app (first pulse moved to step 0)
inline void Euclid(int pulses, int steps, uint8_t *out) {
  if (steps <= 0) return;
  if (pulses <= 0) { memset(out, 0, steps); return; }
  if (pulses >= steps) { memset(out, 1, steps); return; }
  int counts[MAXLEN + 2], rem[MAXLEN + 2];
  int divisor = steps - pulses, level = 0;
  rem[0] = pulses;
  while (true) {
    counts[level] = divisor / rem[level];
    rem[level + 1] = divisor % rem[level];
    divisor = rem[level];
    level++;
    if (rem[level] <= 1) break;
  }
  counts[level] = divisor;
  int n = 0;
  // iterative version of build(l) with an explicit stack
  int st[MAXLEN * 2 + 8]; int sp = 0;
  st[sp++] = level;
  while (sp) {
    const int l = st[--sp];
    if (l == -1) { if (n < steps) out[n++] = 0; continue; }
    if (l == -2) { if (n < steps) out[n++] = 1; continue; }
    // push in reverse order: counts[l] times build(l-1), then build(l-2) if rem
    if (rem[l] != 0) st[sp++] = l - 2;
    for (int i = 0; i < counts[l]; ++i) st[sp++] = l - 1;
  }
  int f = 0; while (f < steps && !out[f]) f++;
  if (f > 0 && f < steps) {
    uint8_t tmp[MAXLEN];
    for (int i = 0; i < steps; ++i) tmp[i] = out[(i + f) % steps];
    memcpy(out, tmp, steps);
  }
}
inline uint32_t Hash(uint32_t a, uint32_t b) {
  uint32_t x = a * 0x9E3779B1u ^ b * 0x85EBCA77u;
  x ^= x >> 15; x *= 0x2C1B3C6Du; x ^= x >> 12; x *= 0x297A2D39u; x ^= x >> 15;
  return x;
}

// ---------------------------------------------------------------- factory patterns
struct Pattern { Track t[NT]; uint8_t swing; };
inline Track Mk(int len, int steps, int hits, int rot, int ratio, int prob, int azar, int burst, int bhits, int nota, int mel, int note, int chan) {
  Track t; t.len = len; t.steps = steps; t.hits = hits; t.rot = rot; t.ratio = ratio; t.prob = prob; t.azar = azar;
  t.burst = burst; t.bhits = bhits; t.nota = nota; t.mel = mel; t.midi_note = note; t.chan = chan;
  return t;
}
static constexpr int NUM_FACTORY = 8;
static const char *const FACTORY_NAMES[NUM_FACTORY + 1] = {"Base 4x4", "Hats 16", "Rolling", "Poli 12/7/5", "Roto", "Hipnotico", "Rafagas", "Doble", "Memoria M"};
inline void Factory(int i, Pattern &p) {
  p.swing = 0;
  switch (i) {
    default:
    case 0: p.t[0] = Mk(16, 16, 4, 0, 4, 100, 0, 1, 0, 0, 0, 36, 11); p.t[1] = Mk(16, 16, 4, 2, 4, 100, 0, 1, 0, 0, 0, 42, 12);
            p.t[2] = Mk(16, 16, 3, 2, 4, 100, 0, 1, 0, 1, M_ESCALA, 48, 13); p.t[3] = Mk(16, 16, 2, 4, 4, 90, 0, 1, 0, 1, M_AZAR, 60, 14); break;
    case 1: p.t[0] = Mk(16, 16, 4, 0, 4, 100, 0, 1, 0, 0, 0, 36, 11); p.t[1] = Mk(16, 16, 16, 0, 4, 85, 30, 1, 0, 0, 0, 42, 12);
            p.t[2] = Mk(16, 16, 5, 1, 4, 100, 0, 1, 0, 1, M_ESCALA, 48, 13); p.t[3] = Mk(16, 16, 4, 2, 4, 100, 0, 1, 0, 0, 0, 39, 14); p.swing = 15; break;
    case 2: p.t[0] = Mk(16, 16, 4, 0, 4, 100, 0, 1, 0, 0, 0, 36, 11); p.t[1] = Mk(16, 16, 12, 1, 4, 90, 20, 2, 2, 0, 0, 42, 12);
            p.t[2] = Mk(16, 16, 13, 0, 4, 100, 0, 1, 0, 1, M_AZAR, 48, 13); p.t[3] = Mk(32, 16, 3, 3, 4, 80, 20, 1, 0, 1, M_ESCALA, 60, 14); break;
    case 3: p.t[0] = Mk(16, 16, 4, 0, 4, 100, 0, 1, 0, 0, 0, 36, 11); p.t[1] = Mk(12, 12, 5, 0, 4, 100, 0, 1, 0, 0, 0, 42, 12);
            p.t[2] = Mk(7, 7, 3, 1, 4, 100, 0, 1, 0, 1, M_ESCALA, 48, 13); p.t[3] = Mk(5, 5, 2, 0, 4, 100, 0, 1, 0, 1, M_AZAR, 60, 14); break;
    case 4: p.t[0] = Mk(16, 16, 4, 0, 4, 100, 0, 1, 0, 0, 0, 36, 11); p.t[1] = Mk(16, 16, 7, 3, 4, 100, 0, 1, 0, 0, 0, 42, 12);
            p.t[2] = Mk(16, 16, 5, 3, 4, 100, 0, 1, 0, 1, M_CURSOR, 48, 13); p.t[3] = Mk(16, 16, 3, 5, 4, 100, 0, 1, 0, 0, 0, 39, 14); break;
    case 5: p.t[0] = Mk(16, 16, 4, 0, 4, 100, 0, 1, 0, 0, 0, 36, 11); p.t[1] = Mk(16, 16, 8, 1, 4, 100, 0, 1, 0, 0, 0, 42, 12);
            p.t[2] = Mk(16, 16, 16, 0, 4, 100, 0, 1, 0, 1, M_FRASE, 48, 13); p.t[3] = Mk(64, 16, 1, 0, 7, 100, 0, 1, 0, 1, M_ESCALA, 60, 14); break;
    case 6: p.t[0] = Mk(16, 16, 4, 0, 4, 100, 0, 2, 1, 0, 0, 36, 11); p.t[1] = Mk(16, 16, 6, 2, 4, 100, 10, 3, 2, 0, 0, 42, 12);
            p.t[2] = Mk(16, 16, 5, 1, 4, 100, 0, 2, 1, 1, M_AZAR, 48, 13); p.t[3] = Mk(16, 16, 3, 7, 4, 100, 0, 4, 1, 0, 0, 39, 14); break;
    case 7: p.t[0] = Mk(16, 16, 4, 0, 4, 100, 0, 1, 0, 0, 0, 36, 11); p.t[1] = Mk(16, 16, 4, 2, 3, 100, 0, 1, 0, 0, 0, 42, 12);
            p.t[2] = Mk(16, 16, 6, 2, 3, 100, 0, 1, 0, 1, M_ESCALA, 48, 13); p.t[3] = Mk(16, 16, 4, 1, 5, 100, 0, 1, 0, 1, M_AZAR, 60, 14); break;
  }
  for (int k = 0; k < NT; ++k) p.t[k].seed = 0x1000u * (i + 1) + k;
}

// ---------------------------------------------------------------- engine
class Engine {
public:
  Track t[NT];
  uint8_t swing = 0, scale = 1, root = 0;          // global: swing %, scale, root (C=0)
  bool midi = true, mclk = false;
  bool mute[NT] = {false, false, false, false};
  bool fill = false;
  int cv_hits[NT] = {0}, cv_rot[NT] = {0}, cv_steps[NT] = {0}, cv_len[NT] = {0}, cv_prob[NT] = {0}, cv_azar[NT] = {0},
      cv_ratio[NT] = {0}, cv_burst[NT] = {0}, cv_pitch[NT] = {0};
  bool cv_mute[NT] = {false, false, false, false};
  // modulation from the Eucl x4 applets, on top of the app's / Brains' (cleared by whoever drives the engine)
  int hm_hits[NT] = {0}, hm_rot[NT] = {0}, hm_steps[NT] = {0}, hm_len[NT] = {0}, hm_prob[NT] = {0}, hm_azar[NT] = {0},
      hm_ratio[NT] = {0}, hm_burst[NT] = {0}, hm_pitch[NT] = {0};
  bool hm_mute[NT] = {false, false, false, false}, hm_fill = false;
  void ClearHM() {
    for (int k = 0; k < NT; ++k) { hm_hits[k] = hm_rot[k] = hm_steps[k] = hm_len[k] = hm_prob[k] = hm_azar[k] = 0;
                                   hm_ratio[k] = hm_burst[k] = hm_pitch[k] = 0; hm_mute[k] = false; }
    hm_fill = false;
  }

  // outputs, valid during the ISR tick after Update()
  bool hit[NT] = {false, false, false, false};
  uint8_t vel[NT] = {0};
  int pitch[NT] = {0};                             // semitones above C(-1); the CV uses pitch - 48 (C3 = 0 V)
  int gate_left[NT] = {0};
  bool clock16 = false, cycle_reset = false;

  void Init() {
    Pattern p; Factory(0, p); LoadPattern(p);
    for (int k = 0; k < NT; ++k) { seq_step[k] = 0; pitch[k] = 48; memset(phrase[k], 48, MAXLEN); }
  }

  // ---- clock: one call per semicorchea; period = ISR ticks since the previous one
  void Clock16(int period) {
    clock16 = true;
    period16 = period;
    if (morph_bars && (clocks % 16) == 0 && clocks > 0) MorphStep();
    for (int k = 0; k < NT; ++k) {
      const Track &tr = t[k];
      const int ri = constrain(tr.ratio + (cv_ratio[k] + hm_ratio[k]), 0, 8);
      const int n = RATIO_N[ri], d = RATIO_D[ri];
      if (d > 1) {
        if ((clocks % d) == 0) Schedule(k, 0, period * d);
      } else {
        for (int i = 0; i < n; ++i) Schedule(k, period * i / n, period / n);
      }
    }
    clocks++;
    if (mclk && midi) mclk_left = 6, mclk_timer = 0, mclk_step = period / 6;
  }
  void Reset() {
    for (int k = 0; k < NT; ++k) { seq_step[k] = 0; bite[k] = 0; }
    clocks = 0;
    for (int i = 0; i < QN; ++i) q_time[i] = -1;
    cycle_reset = true;
  }

  // ---- every ISR tick
  void Update() {
    for (int k = 0; k < NT; ++k) { hit[k] = false; if (gate_left[k] > 0) gate_left[k]--; }
    for (int i = 0; i < QN; ++i) {
      if (q_time[i] < 0 || --q_time[i] > 0) continue;
      q_time[i] = -1;
      if (q_kind[i] == 0) Step(q_track[i], q_dur[i]);
      else if (q_kind[i] == 1) Fire(q_track[i], q_vel[i], q_note[i], q_dur[i]);
      else if (q_kind[i] == 2) { if (midi) usbMIDI.sendNoteOff(q_note[i], 0, t[q_track[i]].chan); }
    }
    if (mclk_left > 0 && --mclk_timer <= 0) {
      usbMIDI.sendRealTime(usbMIDI.Clock); mclk_left--; mclk_timer = mclk_step > 0 ? mclk_step : 1;
      if (mclk_step == 0) mclk_left = 0;
    }
  }
  // call once per ISR tick after reading outputs
  void EndTick() { clock16 = false; cycle_reset = false; }

  // ---- patterns, morph
  void GetPattern(Pattern &p) const { for (int k = 0; k < NT; ++k) p.t[k] = t[k]; p.swing = swing; }
  void LoadPattern(const Pattern &p) { for (int k = 0; k < NT; ++k) t[k] = p.t[k]; swing = p.swing; morph_bars = 0; }
  // morph the rhythm towards p over `bars` bars (0 = jump)
  void MorphTo(const Pattern &p, int bars) {
    if (bars <= 0) { // keep notes / MIDI settings of the target too
      LoadPattern(p); return;
    }
    GetPattern(morph_from); morph_to = p; morph_bars = bars; morph_k = 0;
  }
  bool Morphing() const { return morph_bars > 0; }
  void NewSeed(int k) { t[k].seed = Hash(t[k].seed ^ micros(), 0x5EED + k); }

  int Hits(int k) const { return constrain(t[k].hits + (cv_hits[k] + hm_hits[k]), 0, Steps(k)); }
  int Steps(int k) const { return constrain(t[k].steps + (cv_steps[k] + hm_steps[k]), 1, MAXLEN); }
  int Len(int k) const { return constrain(t[k].len + (cv_len[k] + hm_len[k]), 1, MAXLEN); }
  int Rot(int k) const { const int s = Steps(k); return (((t[k].rot + (cv_rot[k] + hm_rot[k])) % s) + s) % s; }
  int StepPos(int k) const { return seq_step[k]; }
  bool PatternBit(int k, int i) const {
    uint8_t pat[MAXLEN]; Euclid(Hits(k), Steps(k), pat);
    const int s = Steps(k);
    return pat[(i % s + Rot(k)) % s];
  }
  // CV for a track's pitch (1V/oct, 128 per semitone), C3 = 0 V
  int PitchCV(int k) const { return (pitch[k] - 48) * 128; }

private:
  static constexpr int QN = 48;
  int q_time[QN], q_dur[QN];
  uint8_t q_kind[QN], q_track[QN], q_vel[QN], q_note[QN];
  int seq_step[NT] = {0}, bite[NT] = {0};
  uint32_t clocks = 0;
  int period16 = 1000;
  uint8_t phrase[NT][MAXLEN];
  int mclk_left = 0, mclk_timer = 0, mclk_step = 0;
  Pattern morph_from, morph_to; int morph_bars = 0, morph_k = 0;
  uint32_t rng = 0xE0C11Du;
  uint32_t Rand() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
public:
  Engine() { for (int i = 0; i < QN; ++i) q_time[i] = -1; }
private:

  void Enq(uint8_t kind, int delay, int k, int dur, uint8_t v = 0, uint8_t note = 0) {
    for (int i = 0; i < QN; ++i) if (q_time[i] < 0) {
      // delay 0 runs in the next Update() (same ISR tick when called from Clock16), so the hit is not cleared before it is read
      q_time[i] = delay > 0 ? delay : 1; q_kind[i] = kind; q_track[i] = k; q_dur[i] = dur; q_vel[i] = v; q_note[i] = note;
      return;
    }
  }
  void Schedule(int k, int delay, int stepdur) {
    // swing delays the odd steps
    const int si = seq_step[k] % Len(k);
    if (swing && (si & 1)) delay += stepdur * swing * 65 / 10000;
    Enq(0, delay, k, stepdur);
  }

  bool ShouldTrig(int k, int si) {
    const Track &tr = t[k];
    int p = constrain(tr.prob + (cv_prob[k] + hm_prob[k]), 0, 100);
    const int az = constrain(tr.azar + (cv_azar[k] + hm_azar[k]), 0, 100);
    if (az > 0) {
      const int r = tr.seed_libre ? (int)(Rand() % 1000) : (int)(Hash(tr.seed, si) % 1000);
      p = constrain(p + (r - 500) * az / 1000, 0, 100);
    }
    return (int)(Rand() % 100) < p;
  }

  void Step(int k, int stepdur) {
    const int L = Len(k);
    const int si = seq_step[k] % L;
    advance(k);
    if (bite[k] > 0) { bite[k]--; return; }
    if (k == 0 && si == 0) cycle_reset = true;
    const int s = Steps(k);
    uint8_t pat[MAXLEN]; Euclid(Hits(k), s, pat);
    const int eu = si % s;
    const bool pulse = (fill || hm_fill) ? true : pat[(eu + Rot(k)) % s];
    if (!pulse || mute[k] || cv_mute[k] || hm_mute[k] || !ShouldTrig(k, si)) return;

    const int note = NoteFor(k, si, eu);
    const uint8_t v = 100;
    const int gl = t[k].nota ? stepdur * constrain(t[k].gate, 1, 100) / 100 : 0;
    Fire(k, v, note, gl);

    // bursts on some of the pattern's hits
    const int cnt = constrain(t[k].burst + (cv_burst[k] + hm_burst[k]), 1, 8);
    if (cnt > 1 && InBurstSet(k, pat, s, eu)) {
      const int extras = cnt - 1;
      if (t[k].bmode == 0) {
        const int sp = t[k].bdiv ? period16 * 4 / BDIV[t[k].bdiv] : stepdur / cnt;
        for (int i = 1; i <= extras; ++i) Enq(1, sp * i, k, gl, (uint8_t)(v * (100 - i * 12) / 100), note);
      } else {
        for (int i = 1; i <= extras; ++i) Enq(1, stepdur * i, k, gl, (uint8_t)(v * (100 - i * 15) / 100), note);
        bite[k] = extras;
      }
    }
  }
  void advance(int k) { seq_step[k]++; if (seq_step[k] >= Len(k)) seq_step[k] = 0; }

  bool InBurstSet(int k, const uint8_t *pat, int s, int eu) const {
    int active = 0; for (int i = 0; i < s; ++i) active += pat[(i + Rot(k)) % s];
    const int bh = constrain(t[k].bhits, 0, active);
    if (!active || bh <= 0) return false;
    uint8_t bp[MAXLEN]; Euclid(bh, active, bp);
    int ai = 0;
    for (int i = 0; i < s; ++i) {
      if (!pat[(i + Rot(k)) % s]) continue;
      if (i == eu) return bp[(ai + t[k].brot) % active];
      ai++;
    }
    return false;
  }

  int Quantize(int cursor) const {     // cursor 0..127 over two octaves, like the web app
    const int target = 48 + root + cursor * 24 / 127;
    int best = 48 + root, bd = 999;
    for (int o = -1; o < 3; ++o) for (int i = 0; i < SCALE_LEN[scale]; ++i) {
      const int n = 48 + root + SCALES[scale][i] + 12 * o;
      const int dd = n > target ? n - target : target - n;
      if (dd < bd) { bd = dd; best = n; }
    }
    return best;
  }
  int Degree(int idx) const {
    const int n = SCALE_LEN[scale];
    const int o = idx >= 0 ? idx / n : -((-idx + n - 1) / n);
    return 48 + root + SCALES[scale][((idx % n) + n) % n] + 12 * o;
  }
  int NoteFor(int k, int si, int eu) {
    const Track &tr = t[k];
    int note;
    switch (tr.mel) {
      case M_AZAR: {
        const int az = constrain(tr.azar + (cv_azar[k] + hm_azar[k]), 0, 100);
        if (az > 0) {
          const uint32_t r = tr.seed_libre ? Rand() : Hash(tr.seed ^ 0xA2A2u, eu);
          note = Degree((int)(r % (SCALE_LEN[scale] * 2)));
        } else {
          note = Degree(eu);
        }
        phrase[k][si] = (uint8_t)note;
        break;
      }
      case M_ESCALA: note = Degree(eu); phrase[k][si] = (uint8_t)note; break;
      case M_FRASE: note = phrase[k][si]; break;
      default: note = Quantize(tr.tono); phrase[k][si] = (uint8_t)note; break;
    }
    // CV "Tono": 1V/oct inside the scale (one scale degree per semitone of CV)
    if (cv_pitch[k] + hm_pitch[k]) {
      const int deg = (cv_pitch[k] + hm_pitch[k]) * SCALE_LEN[scale] / 12;
      int best = 0;
      for (int i = 0; i < SCALE_LEN[scale]; ++i) if (Degree(i) % 12 == note % 12) { best = i; break; }
      note = Degree(best + deg) + (note - Degree(best));
    }
    note += 12 * tr.oct;
    return constrain(note, 0, 127);
  }

  void Fire(int k, uint8_t v, int note, int gate_len) {
    hit[k] = true; vel[k] = v; pitch[k] = note;
    gate_left[k] = gate_len > 0 ? gate_len : 0;
    if (!midi) return;
    const Track &tr = t[k];
    if (!tr.nota) {
      usbMIDI.sendNoteOn(tr.midi_note, v, tr.chan);
      Enq(2, 17 * 25, k, 0, 0, tr.midi_note);
    } else {
      const int dur = gate_len > 17 * 10 ? gate_len : 17 * 60;
      for (int i = 0; i < 4 && CHORDS[tr.chord][i] >= 0; ++i) {
        const int n = constrain(note + CHORDS[tr.chord][i], 0, 127);
        usbMIDI.sendNoteOn(n, v, tr.chan);
        Enq(2, dur, k, 0, 0, (uint8_t)n);
      }
    }
  }

  void MorphStep() {
    morph_k++;
    const int K = morph_k, N = morph_bars;
    for (int k = 0; k < NT; ++k) {
      uint8_t *a = (uint8_t *)&morph_from.t[k], *b = (uint8_t *)&morph_to.t[k], *c = (uint8_t *)&t[k];
      for (int f = 0; f < TRACK_NUMERIC; ++f) c[f] = (uint8_t)(a[f] + ((int)b[f] - (int)a[f]) * K / N);
    }
    swing = (uint8_t)(morph_from.swing + ((int)morph_to.swing - (int)morph_from.swing) * K / N);
    if (K >= N) { LoadPattern(morph_to); }
  }
};

} // namespace euclid4

// the one shared instance: the app edits it, the applet and Brains Seq play it
extern euclid4::Engine euclid_engine;
