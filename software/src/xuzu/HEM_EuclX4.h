// Eucl x4 applet (xuzu v4.18): plays and edits the 4 tracks of the Euclid x4 app.
// TR1 clock, TR2 reset. All 4 tracks go out over MIDI (ch 11-14); one track goes to CV:
// output 1 trigger or gate, output 2 pitch. CV1 / CV2 assignable to any track (or all).
// What you edit here is the app's pattern: it is kept with the app and its presets.
#pragma once
#include "xuzu/euclid4_engine.h"
#include "xuzu/hem_list.h"
#include "xuzu/xuzu_flash.h"

class EuclX4 : public XuzuListApplet {
public:
  enum Prm { P_TRACK, P_HITS, P_STEPS, P_LEN, P_ROT, P_RATIO, P_PROB, P_AZAR, P_BURST, P_MUTE,
             P_CVTRACK, P_OUT1, P_PRESET, P_CV1, P_CV1T, P_CV2, P_CV2T, NUM_P };
  enum CvDest { CV_NADA = 0, CV_HITS, CV_ROT, CV_STEPS, CV_LEN, CV_PROB, CV_AZAR, CV_RATIO, CV_BURST, CV_TONO,
                CV_MUTE, CV_FILL, CV_PRESET, NUM_CV };

  const char *applet_name() { return "Eucl x4"; }
  void Start() { cv_track = 2; }

  void Controller() {
    euclid4::Engine &e = euclid_engine;
    const uint32_t now = OC::CORE::ticks;
    // with an Eucl x4 on each side, only one of them runs the shared engine; with a Brains applet, Brains runs it
    const bool owner = (last_owner_tick != now);
    Modulation();
    if (owner) {
      last_owner_tick = now;
      xuzu::eucl_seen_tick = now;
      e.ClearHM();
      for (int h = 0; h < 2; ++h) Sum(e, mod[h]);
      if (!xuzu::Recent(xuzu::brains_seen_tick, now)) {
        e.EndTick();          // clears the flags of the previous tick; the other side reads this tick's after us
        if (since < 1000000) since++;
        if (Clock(1)) e.Reset();
        if (Clock(0)) {
          if (since > 16667) { e.Reset(); e.Clock16(last); } else { e.Clock16(since); last = since; }
          since = 0;
        }
        e.Update();
      }
    }
    const int k = cv_track;
    const bool gate = out1 == 2 || (out1 == 0 && e.t[k].nota);
    if (gate) GateOut(0, e.gate_left[k] > 0 || e.hit[k]);
    else if (e.hit[k]) ClockOut(0);
    Out(1, e.PitchCV(k));
    if (e.hit[k]) flash = 600;
    if (flash) flash--;
  }
  void View() { DrawList(flash > 0); }
  void OnEncoderMove(int dir) { ListEncoder(dir); }

  uint64_t OnDataRequest() {
    uint64_t d = 0;
    Pack(d, PackLocation{0, 2}, cv_track); Pack(d, PackLocation{2, 2}, track); Pack(d, PackLocation{4, 2}, out1);
    Pack(d, PackLocation{6, 5}, preset); Pack(d, PackLocation{11, 4}, cv_dest[0]); Pack(d, PackLocation{15, 3}, cv_trk[0]);
    Pack(d, PackLocation{18, 4}, cv_dest[1]); Pack(d, PackLocation{22, 3}, cv_trk[1]);
    return d;
  }
  void OnDataReceive(uint64_t d) {
    cv_track = Unpack(d, PackLocation{0, 2}); track = Unpack(d, PackLocation{2, 2}); out1 = constrain((int)Unpack(d, PackLocation{4, 2}), 0, 2);
    preset = constrain((int)Unpack(d, PackLocation{6, 5}), 0, 16);
    cv_dest[0] = constrain((int)Unpack(d, PackLocation{11, 4}), 0, NUM_CV - 1); cv_trk[0] = constrain((int)Unpack(d, PackLocation{15, 3}), 0, 4);
    cv_dest[1] = constrain((int)Unpack(d, PackLocation{18, 4}), 0, NUM_CV - 1); cv_trk[1] = constrain((int)Unpack(d, PackLocation{22, 3}), 0, 4);
    if (preset > 0) xuzu::EuclidLoadPreset(preset - 1);     // the Hemisphere preset brings its Euclid pattern
  }

protected:
  void SetHelp() {
    help[HELP_DIGITAL1] = "Clock";
    help[HELP_DIGITAL2] = "Reset";
    help[HELP_CV1] = CV_SHORT[cv_dest[0]];
    help[HELP_CV2] = CV_SHORT[cv_dest[1]];
    help[HELP_OUT1] = "Trig";
    help[HELP_OUT2] = "Tono";
    help[HELP_EXTRA1] = "Pistas de la app Euclid";
    help[HELP_EXTRA2] = "4 por MIDI, 1 por CV";
  }

  // top row: the pattern of the track being edited, with the playhead
  bool Top() {
    euclid4::Engine &e = euclid_engine;
    const int t = track, L = e.Len(t), pos = e.StepPos(t), start = (pos / 16) * 16;
    gfxPrint(1, 15, t + 1);
    for (int i = 0; i < 16 && start + i < L; ++i) {
      const int x = 9 + i * 3;
      if (e.PatternBit(t, (start + i) % e.Steps(t))) gfxRect(x, 16, 2, 6); else gfxPixel(x, 21);
      if (start + i == pos) gfxLine(x, 23, x + 1, 23);
    }
    return true;
  }
  int NumParams() { return NUM_P; }
  const char *Name(int i) {
    static const char *const N[NUM_P] = {"Pista", "Golpe", "Pasos", "Largo", "Rotac", "Ratio", "Prob", "Azar", "Rafag", "Mute",
                                         "CVpis", "Sal 1", "Prest", "CV1", "CV1 p", "CV2", "CV2 p"};
    return N[i];
  }
  void Value(int i, char *b, bool full) {
    euclid4::Engine &e = euclid_engine; const euclid4::Track &t = e.t[track];
    const int m = full ? 9 : 5;
    switch (i) {
      case P_TRACK: xuzu::Copy(b, "T", 1); xuzu::Itoa(track + 1, b + 1); break;
      case P_HITS: xuzu::Itoa(t.hits, b); break;
      case P_STEPS: xuzu::Itoa(t.steps, b); break;
      case P_LEN: xuzu::Itoa(t.len, b); break;
      case P_ROT: xuzu::Itoa(t.rot, b); break;
      case P_RATIO: xuzu::Copy(b, euclid4::RATIO_NAMES[t.ratio], m); break;
      case P_PROB: xuzu::Itoa(t.prob, b); break;
      case P_AZAR: xuzu::Itoa(t.azar, b); break;
      case P_BURST: xuzu::Itoa(t.burst, b); break;
      case P_MUTE: xuzu::Copy(b, e.mute[track] ? "Si" : "No", m); break;
      case P_CVTRACK: xuzu::Copy(b, "T", 1); xuzu::Itoa(cv_track + 1, b + 1); break;
      case P_OUT1: { static const char *const O[3] = {"Auto", "Trig", "Gate"}; xuzu::Copy(b, O[out1], m); break; }
      case P_PRESET: if (!preset) xuzu::Copy(b, "---", m); else { xuzu::Itoa(preset, b); if (full) { const int l = strlen(b); xuzu::Copy(b + l, " cargado", 9 - l); } } break;
      case P_CV1: case P_CV2: { const int c = i == P_CV1 ? 0 : 1; xuzu::Copy(b, full ? CV_NAMES[cv_dest[c]] : CV_SHORT[cv_dest[c]], m); break; }
      case P_CV1T: case P_CV2T: { const int c = i == P_CV1T ? 0 : 1;
        if (cv_trk[c] == 4) xuzu::Copy(b, "Todas", m); else { xuzu::Copy(b, "T", 1); xuzu::Itoa(cv_trk[c] + 1, b + 1); } break; }
    }
  }
  void Change(int i, int dir) {
    euclid4::Engine &e = euclid_engine; euclid4::Track &t = e.t[track];
    switch (i) {
      case P_TRACK: track = constrain(track + dir, 0, 3); break;
      case P_HITS: t.hits = constrain(t.hits + dir, 0, t.steps); break;
      case P_STEPS: t.steps = constrain(t.steps + dir, 1, euclid4::MAXLEN); if (t.hits > t.steps) t.hits = t.steps; break;
      case P_LEN: t.len = constrain(t.len + dir, 1, euclid4::MAXLEN); break;
      case P_ROT: t.rot = constrain(t.rot + dir, 0, t.steps - 1); break;
      case P_RATIO: t.ratio = constrain(t.ratio + dir, 0, 8); break;
      case P_PROB: t.prob = constrain(t.prob + dir * 5, 0, 100); break;
      case P_AZAR: t.azar = constrain(t.azar + dir * 5, 0, 100); break;
      case P_BURST: t.burst = constrain(t.burst + dir, 1, 8); break;
      case P_MUTE: e.mute[track] = dir > 0; break;
      case P_CVTRACK: cv_track = constrain(cv_track + dir, 0, 3); break;
      case P_OUT1: out1 = constrain(out1 + dir, 0, 2); break;
      case P_PRESET: preset = constrain(preset + dir, 0, 16); if (preset > 0) xuzu::EuclidLoadPreset(preset - 1); break;
      case P_CV1: case P_CV2: { const int c = i == P_CV1 ? 0 : 1; cv_dest[c] = constrain(cv_dest[c] + dir, 0, NUM_CV - 1); break; }
      case P_CV1T: case P_CV2T: { const int c = i == P_CV1T ? 0 : 1; cv_trk[c] = constrain(cv_trk[c] + dir, 0, 4); break; }
    }
  }

private:
  static constexpr const char *CV_NAMES[NUM_CV] = {"Nada", "Golpes", "Rotacion", "Pasos", "Largo", "Probabil.", "Azar",
    "Ratio", "Rafaga", "Tono", "Mute", "Fill", "Preset"};
  static constexpr const char *CV_SHORT[NUM_CV] = {"---", "Golpe", "Rotac", "Pasos", "Largo", "Prob", "Azar", "Ratio",
    "Rafag", "Tono", "Mute", "Fill", "Prest"};
  struct Mod { int16_t hits[4], rot[4], steps[4], len[4], prob[4], azar[4], ratio[4], burst[4], pitch[4]; bool mute[4], fill; };
  static inline Mod mod[2] = {};
  static inline uint32_t last_owner_tick = 0xFFFFFFFFu;

  int track = 0, cv_track = 2, out1 = 0, preset = 0;
  int cv_dest[2] = {CV_NADA, CV_NADA}, cv_trk[2] = {2, 2};
  int cv_preset_last = -1;
  int since = 1000000, last = 2000, flash = 0;

  // this side's CV modulation, kept apart so two applets can add theirs
  void Modulation() {
    Mod &m = mod[hemisphere & 1];
    m = Mod();
    for (int c = 0; c < 2; ++c) {
      const int cv = In(c), dest = cv_dest[c];
      if (dest == CV_NADA) continue;
      if (dest == CV_PRESET) {
        const int s = xuzu::CVPreset(cv, 7680, cv_preset_last);
        if (s >= 0) { preset = s + 1; xuzu::EuclidLoadPreset(s); }
        continue;
      }
      if (dest == CV_FILL) { m.fill |= cv > HEMISPHERE_3V_CV / 2; continue; }
      for (int k = 0; k < 4; ++k) {
        if (cv_trk[c] != 4 && cv_trk[c] != k) continue;
        switch (dest) {
          case CV_HITS: m.hits[k] += xuzu::Bi(cv, 16); break;
          case CV_ROT: m.rot[k] += xuzu::Bi(cv, 16); break;
          case CV_STEPS: m.steps[k] += xuzu::Bi(cv, 16); break;
          case CV_LEN: m.len[k] += xuzu::Bi(cv, 16); break;
          case CV_PROB: m.prob[k] += xuzu::Bi(cv, 100); break;
          case CV_AZAR: m.azar[k] += xuzu::Bi(cv, 100); break;
          case CV_RATIO: m.ratio[k] -= xuzu::Bi(cv, 4); break;
          case CV_BURST: m.burst[k] += xuzu::Bi(cv, 7); break;
          case CV_TONO: m.pitch[k] += cv / 128; break;
          case CV_MUTE: m.mute[k] |= cv > HEMISPHERE_3V_CV / 2; break;
        }
      }
    }
  }
  static void Sum(euclid4::Engine &e, const Mod &m) {
    for (int k = 0; k < 4; ++k) {
      e.hm_hits[k] += m.hits[k]; e.hm_rot[k] += m.rot[k]; e.hm_steps[k] += m.steps[k]; e.hm_len[k] += m.len[k];
      e.hm_prob[k] += m.prob[k]; e.hm_azar[k] += m.azar[k]; e.hm_ratio[k] += m.ratio[k]; e.hm_burst[k] += m.burst[k];
      e.hm_pitch[k] += m.pitch[k]; e.hm_mute[k] = e.hm_mute[k] || m.mute[k];
    }
    e.hm_fill = e.hm_fill || m.fill;
  }
};
