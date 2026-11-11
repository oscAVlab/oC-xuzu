// Compas applet (xuzu v4.18): a Compas of its own in half a screen, with all its settings.
// TR1 clock (x4 or x2 of the Pam)  TR2 reset   CV1 / CV2 assignable (Dens / Solt by default)
// Outputs: Trig+Mel (default), A+B, C+D, Acen+Rell, Raiz+Mel (in the electronic styles the root follows their bass line)
// MIDI like the app: ch10 percussion, ch1 melody, ch2 chords, ch3 bass
#pragma once
#include "xuzu/compas_engine.h"
#include "xuzu/hem_list.h"

class CompasApplet : public XuzuListApplet {
public:
  enum Prm { P_STYLE, P_FAM, P_PROG, P_DENS, P_SOLT, P_NOTAS, P_ADORNO, P_EMPUJE, P_RANGO, P_OCT, P_LOCK, P_LOOP,
             P_OUTS, P_CV1, P_CV2, P_CLK, P_MIDI, P_MAT, NUM_P };
  enum Outs { O_TRIGMEL = 0, O_AB, O_CD, O_ACCFILL, O_ROOTMEL, NUM_OUTS };
  enum CvDest { CV_NADA = 0, CV_DENS, CV_SOLT, CV_NOTAS, CV_ADORNO, CV_EMPUJE, CV_ESTILO, CV_PROG, CV_RANGO, CV_OCT,
                CV_TRANSP, CV_CONGELAR, CV_NUEVO, CV_LOOP, NUM_CV };

  const char *applet_name() { return "Compas"; }

  void Start() {
    mat = (int)((OC::CORE::ticks * 7u + (uint32_t)hemisphere * 13u) % 32u);
    engine.Restart(Seed());
  }

  void Controller() {
    ApplyParams();
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
    if (flash) flash--;
  }

  void View() { DrawList(flash > 0); }
  void OnEncoderMove(int dir) { ListEncoder(dir); }

  uint64_t OnDataRequest() {
    uint64_t d = 0;
    Pack(d, PackLocation{0, 5}, style); Pack(d, PackLocation{5, 5}, dens / 5); Pack(d, PackLocation{10, 5}, solt / 5);
    Pack(d, PackLocation{15, 5}, notas / 5); Pack(d, PackLocation{20, 5}, adorno / 5); Pack(d, PackLocation{25, 5}, empuje / 5);
    Pack(d, PackLocation{30, 3}, fam); Pack(d, PackLocation{33, 4}, prog); Pack(d, PackLocation{37, 3}, rango);
    Pack(d, PackLocation{40, 3}, oct + 3); Pack(d, PackLocation{43, 3}, loop); Pack(d, PackLocation{46, 3}, outs);
    Pack(d, PackLocation{49, 4}, cv_dest[0]); Pack(d, PackLocation{53, 4}, cv_dest[1]);
    Pack(d, PackLocation{57, 1}, clk); Pack(d, PackLocation{58, 1}, midi ? 0 : 1); Pack(d, PackLocation{59, 5}, mat);
    return d;
  }
  void OnDataReceive(uint64_t d) {
    style = constrain((int)Unpack(d, PackLocation{0, 5}), 0, compas::NUM_STYLES - 1);
    dens = Pct(Unpack(d, PackLocation{5, 5})); solt = Pct(Unpack(d, PackLocation{10, 5}));
    notas = Pct(Unpack(d, PackLocation{15, 5})); adorno = Pct(Unpack(d, PackLocation{20, 5})); empuje = Pct(Unpack(d, PackLocation{25, 5}));
    fam = constrain((int)Unpack(d, PackLocation{30, 3}), 0, compas::NUM_FAMS - 1);
    prog = Unpack(d, PackLocation{33, 4}); rango = Unpack(d, PackLocation{37, 3});
    oct = (int)Unpack(d, PackLocation{40, 3}) - 3; loop = Unpack(d, PackLocation{43, 3});
    outs = constrain((int)Unpack(d, PackLocation{46, 3}), 0, NUM_OUTS - 1);
    cv_dest[0] = constrain((int)Unpack(d, PackLocation{49, 4}), 0, NUM_CV - 1);
    cv_dest[1] = constrain((int)Unpack(d, PackLocation{53, 4}), 0, NUM_CV - 1);
    clk = Unpack(d, PackLocation{57, 1}); midi = !Unpack(d, PackLocation{58, 1}); mat = Unpack(d, PackLocation{59, 5});
    lock = 0;
    ApplyParams();
    engine.Restart(Seed());     // the preset brings back the same material
  }

protected:
  void SetHelp() {
    help[HELP_DIGITAL1] = "Clock";
    help[HELP_DIGITAL2] = "Reset";
    help[HELP_CV1] = CV_SHORT[cv_dest[0]];
    help[HELP_CV2] = CV_SHORT[cv_dest[1]];
    help[HELP_OUT1] = outs == O_ROOTMEL ? "Raiz" : "Trig";
    help[HELP_OUT2] = (outs == O_ACCFILL) ? "Rell" : (outs == O_CD ? "Raiz" : "Melod");
    help[HELP_EXTRA1] = "Reloj: x4 o x2 del Pam";
    help[HELP_EXTRA2] = "MIDI: 10 perc 1 mel 2 3";
  }

  int NumParams() { return NUM_P; }
  const char *Name(int i) {
    static const char *const N[NUM_P] = {"Estil", "Armon", "Progr", "Dens", "Solt", "Notas", "Adorn", "Empuj", "Rango", "Octav",
                                         "Congl", "Loop", "Salid", "CV1", "CV2", "Reloj", "MIDI", "Mater"};
    return N[i];
  }
  void Value(int i, char *b, bool full) {
    const int m = full ? 9 : 5;
    switch (i) {
      case P_STYLE: xuzu::Copy(b, compas::STYLES[style].name, m); break;
      case P_FAM: xuzu::Copy(b, full ? compas::FAMS[fam].name : compas::FAMS[fam].abbr, m); break;
      case P_PROG: xuzu::Copy(b, compas::Engine::ProgName(fam, fam == compas::H_AUTO ? 1 : prog), m); break;
      case P_DENS: xuzu::Itoa(dens, b); break;
      case P_SOLT: xuzu::Itoa(solt, b); break;
      case P_NOTAS: xuzu::Itoa(notas, b); break;
      case P_ADORNO: xuzu::Itoa(adorno, b); break;
      case P_EMPUJE: xuzu::Itoa(empuje, b); break;
      case P_RANGO: xuzu::Itoa(RANGOS[rango], b); break;
      case P_OCT: xuzu::Signed(oct, b); break;
      case P_LOCK: { static const char *const L[compas::NUM_LOCKS] = {"No", "Ritmo", "Melod", "Armon", "R+M", "Todo"}; xuzu::Copy(b, L[lock], m); break; }
      case P_LOOP: { static const char *const L[compas::NUM_LOOPS] = {"1 t", "2 t", "4 t", "1/2 c", "1 c", "2 c", "4 c", "8 c"}; xuzu::Copy(b, L[loop], m); break; }
      case P_OUTS: { static const char *const O[NUM_OUTS] = {"Trig+Mel", "A+B", "C+D", "Acen+Rell", "Raiz+Mel"};
                     static const char *const S[NUM_OUTS] = {"T+Mel", "A+B", "C+D", "Ac+Re", "R+Mel"}; xuzu::Copy(b, full ? O[outs] : S[outs], m); break; }
      case P_CV1: case P_CV2: xuzu::Copy(b, full ? CV_NAMES[cv_dest[i - P_CV1]] : CV_SHORT[cv_dest[i - P_CV1]], m); break;
      case P_CLK: xuzu::Copy(b, clk ? "x2 Pam" : "x4 Pam", m); break;
      case P_MIDI: xuzu::Copy(b, midi ? "Si" : "No", m); break;
      case P_MAT: xuzu::Itoa(mat + 1, b); if (full) { const int l = strlen(b); xuzu::Copy(b + l, " Nuevo>", 9 - l); } break;
    }
  }
  void Change(int i, int dir) {
    switch (i) {
      case P_STYLE: style = constrain(style + dir, 0, compas::NUM_STYLES - 1); break;
      case P_FAM: fam = constrain(fam + dir, 0, compas::NUM_FAMS - 1); prog = 1; break;
      case P_PROG: prog = constrain(prog + dir, 0, compas::Engine::NumProgs(fam) - 1); break;
      case P_DENS: dens = constrain(dens + dir * 5, 0, 100); break;
      case P_SOLT: solt = constrain(solt + dir * 5, 0, 100); break;
      case P_NOTAS: notas = constrain(notas + dir * 5, 0, 100); break;
      case P_ADORNO: adorno = constrain(adorno + dir * 5, 0, 100); break;
      case P_EMPUJE: empuje = constrain(empuje + dir * 5, 0, 100); break;
      case P_RANGO: rango = constrain(rango + dir, 0, 7); break;
      case P_OCT: oct = constrain(oct + dir, -3, 4); break;
      case P_LOCK: lock = constrain(lock + dir, 0, compas::NUM_LOCKS - 1); break;
      case P_LOOP: loop = constrain(loop + dir, 0, compas::NUM_LOOPS - 1); break;
      case P_OUTS: outs = constrain(outs + dir, 0, NUM_OUTS - 1); break;
      case P_CV1: case P_CV2: cv_dest[i - P_CV1] = constrain(cv_dest[i - P_CV1] + dir, 0, NUM_CV - 1); break;
      case P_CLK: clk = constrain(clk + dir, 0, 1); break;
      case P_MIDI: midi = constrain(midi + dir, 0, 1); break;
      case P_MAT: mat = (mat + dir + 32) % 32; engine.Restart(Seed()); break;    // each number is its own material
    }
  }

private:
  static constexpr uint8_t RANGOS[8] = {6, 8, 10, 12, 14, 16, 18, 21};
  static constexpr const char *CV_NAMES[NUM_CV] = {"Nada", "Densidad", "Soltura", "Notas", "Adorno", "Empuje", "Estilo",
    "Progresion", "Rango", "Octava", "Transpone", "Congelar", "Nuevo", "Loop"};
  static constexpr const char *CV_SHORT[NUM_CV] = {"---", "Dens", "Solt", "Notas", "Adorn", "Empuj", "Estil", "Progr",
    "Rango", "Octav", "Trans", "Congl", "Nuevo", "Loop"};

  compas::Engine engine;
  int style = 0, fam = 0, prog = 1, dens = 55, solt = 15, notas = 50, adorno = 30, empuje = 20, rango = 2, oct = 0;
  int lock = 0, loop = 4, outs = O_TRIGMEL, clk = 0, midi = 1, mat = 0;
  int cv_dest[2] = {CV_DENS, CV_SOLT};
  int transp = 0, oct_eff = 0; bool nuevo_hi = false;
  int since = 1000000, count = 0, half = 0, last_tp = 1000, flash = 0;
  int8_t mel_note = -1, bass_note = -1; int bass_semis = 0;
  int mel_semis = 12, glide_left = 0, glide_len = 1, glide_from = 0; bool slide_next = false;
  int8_t chord_on[4] = {-1, -1, -1, -1};
  enum { K_ACC = 1, K_FILL, K_MEL, K_OFF, K_BASS };
  static constexpr int QN = 24;
  int q_t[QN] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  uint8_t q_k[QN]; int16_t q_v[QN]; uint8_t q_a[QN];

  static int Pct(int v) { return constrain(v * 5, 0, 100); }
  uint32_t Seed() const { return 0xC0A5F00Du ^ ((uint32_t)(mat + 1) * 2654435761u); }

  // the settings plus what CV1 / CV2 add, every ISR tick
  void ApplyParams() {
    int st = style, pg = prog, de = dens, so = solt, no = notas, ad = adorno, em = empuje, ra = rango, lk = lock, lp = loop;
    transp = 0; oct_eff = oct;
    bool nuevo = false;
    for (int c = 0; c < 2; ++c) {
      const int cv = In(c);
      switch (cv_dest[c]) {
        case CV_DENS: de += xuzu::Bi(cv, 100); break;
        case CV_SOLT: so += xuzu::Bi(cv, 100); break;
        case CV_NOTAS: no += xuzu::Bi(cv, 100); break;
        case CV_ADORNO: ad += xuzu::Bi(cv, 100); break;
        case CV_EMPUJE: em += xuzu::Bi(cv, 100); break;
        case CV_ESTILO: st = (st + xuzu::Uni(cv, compas::NUM_STYLES)) % compas::NUM_STYLES; break;
        case CV_PROG: { const int n = compas::Engine::NumProgs(fam); pg = n > 1 ? 1 + (pg - 1 + xuzu::Uni(cv, n - 1)) % (n - 1) : pg; break; }
        case CV_RANGO: ra += xuzu::Bi(cv, 7); break;
        case CV_OCT: oct_eff += cv >= 0 ? cv / 1536 : -((-cv + 1535) / 1536); break;
        case CV_TRANSP: transp += cv >= 0 ? (cv + 64) / 128 : -((-cv + 64) / 128); break;
        case CV_CONGELAR: if (cv > HEMISPHERE_3V_CV / 2) lk = compas::L_TODO; break;
        case CV_NUEVO: nuevo = cv > HEMISPHERE_3V_CV / 2; break;
        case CV_LOOP: lp += xuzu::Bi(cv, 7); break;
        default: break;
      }
    }
    if (nuevo && !nuevo_hi) { mat = (mat + 1) % 32; engine.Restart(Seed()); }
    nuevo_hi = nuevo;
    engine.style = st; engine.fam = fam; engine.prog = fam == compas::H_AUTO ? 1 : pg;
    engine.density = constrain(de, 0, 100); engine.soltura = constrain(so, 0, 100); engine.notas = constrain(no, 0, 100);
    engine.adorno = constrain(ad, 0, 100); engine.empuje = constrain(em, 0, 100);
    engine.range = RANGOS[constrain(ra, 0, 7)]; engine.lock = lk;
    lp = constrain(lp, 0, compas::NUM_LOOPS - 1);
    engine.loop_r = lp; engine.loop_m = lp < compas::NUM_LOOPS - 1 ? lp + 1 : lp;
  }

  void Enq(int d, uint8_t k, int16_t v, uint8_t a) {
    if (d <= 0) { Do(k, v, a); return; }
    for (int i = 0; i < QN; ++i) if (q_t[i] < 0) { q_t[i] = d; q_k[i] = k; q_v[i] = v; q_a[i] = a; return; }
  }
  void Tick(int tp) {
    last_tp = tp;
    const compas::Event ev = engine.Tick();
    if (ev.beat_start) flash = 1500;
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
    if (StyleBass()) {                // electronic styles: their own bass line
      for (int i = 0; i < ev.bass_n; ++i) Enq(tp * ev.bass_at[i] / compas::FINE, K_BASS, ev.bass_semis[i], (ev.bass_acc >> i) & 1 ? 120 : 95);
    } else if (midi && (ev.fine_a & 1u)) {   // bass: the root on every accent, chords with the rhythm
      if (bass_note >= 0) usbMIDI.sendNoteOff(bass_note, 0, 3);
      bass_note = (int8_t)constrain(40 + engine.RootSemis() % 12 + transp + 12 * oct_eff, 0, 127); usbMIDI.sendNoteOn(bass_note, 100, 3);
    }
    if (midi && ev.comp) Chord(ev.comp_vel);
    if (midi && ev.remate) { usbMIDI.sendNoteOn(49, 110, 10); usbMIDI.sendNoteOff(49, 0, 10); }
  }
  void Chord(uint8_t vel) {
    for (int i = 0; i < 4; ++i) if (chord_on[i] >= 0) { usbMIDI.sendNoteOff(chord_on[i], 0, 2); chord_on[i] = -1; }
    int8_t iv[4]; const int n = engine.ChordTones(iv);
    const int root = 52 + ((engine.RootSemis() % 12) + 12) % 12 + transp;
    for (int i = 0; i < n; ++i) { chord_on[i] = (int8_t)constrain(root + iv[i], 0, 127); usbMIDI.sendNoteOn(chord_on[i], vel, 2); }
  }
  bool StyleBass() const { return (engine.Def().flags & compas::F_LINE16) && !(engine.Def().flags & compas::F_ACID); }
  bool MelOut() const { return outs == O_TRIGMEL || outs == O_AB || outs == O_ROOTMEL; }
  int Shift() const { return (transp + 12 * oct_eff) * 128; }
  int MelCV() const { const int to = mel_semis * 128; return Shift() + (glide_left > 0 ? to + (glide_from - to) * glide_left / glide_len : to); }
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
          if (slide_next) { glide_from = MelCV() - Shift(); glide_len = glide_left = constrain(last_tp / 4, 17, 1700); }
          else glide_left = 0;
          slide_next = v & 0x200;
        }
        mel_semis = semis;
        if (MelOut()) Out(1, MelCV());
        if (midi) { if (mel_note >= 0) usbMIDI.sendNoteOff(mel_note, 0, 1); mel_note = (int8_t)constrain(64 + semis + transp + 12 * oct_eff, 0, 127); usbMIDI.sendNoteOn(mel_note, a, 1); }
        break;
      }
      case K_OFF: Drum(v, a); break;
      case K_BASS:
        bass_semis = v;
        if (midi) { if (bass_note >= 0) usbMIDI.sendNoteOff(bass_note, 0, 3); bass_note = (int8_t)constrain(40 + v + transp + 12 * oct_eff, 0, 127); usbMIDI.sendNoteOn(bass_note, a, 3); }
        break;
    }
    if (outs == O_CD) Out(1, engine.RootSemis() * 128 + Shift());
    if (outs == O_ROOTMEL) Out(0, (StyleBass() ? bass_semis : engine.RootSemis()) * 128 + Shift());   // electronic: the bass line
  }
};
