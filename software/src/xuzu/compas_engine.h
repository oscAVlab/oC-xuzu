// Compas engine (xuzu rebuild of v4.16, based on the v4.4 source + the 4.5-4.16 changelog)
// rhythm + harmony + melody generator, hardware independent.
// Clock: one tick per corchea; 6 fine steps per corchea (3 = semicorchea, 2/4 = tresillo).
// Styles live on a semicorchea grid; harmony comes from families of progressions that can be
// combined with any style. Loop memory works like a Turing Machine, per layer:
// rhythm, melody and harmony can be frozen separately.
#pragma once
#include <stdint.h>

namespace compas {

static constexpr int FINE = 6;
static constexpr int MAX_LOOP = 8;

// ---------------------------------------------------------------- harmony
// Roots are semitones above E (E=0 F=1 F#=2 G=3 G#=4 A=5 Bb=6 B=7 C=8 C#=9 D=10 D#=11)
enum Quality : uint8_t { MAJ = 0, MIN, DOM7, MAJ7, MIN7, M7B5, DIM7 };
struct Chord { int8_t root; uint8_t q; uint8_t m; };   // m = mode used by the melody

enum ModeKind : uint8_t { PHRYG = 0, MAJOR, HMIN, PENTA, NMIN };
struct Mode { int8_t tonic; uint8_t kind; };
enum { M_E_PHR = 0, M_A_PHR, M_E_MAJ, M_A_HMIN, M_FS_PHR, M_B_PHR, M_A_PENT, M_E_HMIN, M_C_MAJ, M_A_NMIN, M_G_MAJ, NUM_MODES };
static const Mode MODES[NUM_MODES] = {
  {0, PHRYG}, {5, PHRYG}, {0, MAJOR}, {5, HMIN}, {2, PHRYG}, {7, PHRYG},
  {5, PENTA}, {0, HMIN}, {8, MAJOR}, {5, NMIN}, {3, MAJOR}};
static const int8_t SC_PHRYG[7] = {0, 1, 3, 5, 7, 8, 10};
static const int8_t SC_MAJOR[7] = {0, 2, 4, 5, 7, 9, 11};
static const int8_t SC_HMIN[7]  = {0, 2, 3, 5, 7, 8, 11};
static const int8_t SC_NMIN[7]  = {0, 2, 3, 5, 7, 8, 10};
static const int8_t SC_PENTA[5] = {0, 3, 5, 7, 10};
static const char *const NOTE_NAMES[12] = {"E","F","F#","G","G#","A","Bb","B","C","C#","D","Eb"};

struct ProgDef { const char *name; uint8_t len; Chord c[8]; };
#define E_ 0
#define F_ 1
#define FS 2
#define G_ 3
#define GS 4
#define A_ 5
#define BB 6
#define B_ 7
#define C_ 8
#define CS 9
#define D_ 10
#define EB 11

// Flamenca: por arriba (E frigio), por medio (A frigio), taranta, granaina, alegrias, sevillanas
static const ProgDef P_FLAM[] = {
  {"Arriba",  4, {{A_,MIN,0},{G_,MAJ,0},{F_,MAJ,0},{E_,MAJ,0}}},
  {"Arriba8", 8, {{A_,MIN,0},{G_,MAJ,0},{F_,MAJ,0},{E_,MAJ,0},{D_,MIN,0},{A_,MIN,0},{F_,MAJ,0},{E_,MAJ,0}}},
  {"Medio",   4, {{D_,MIN,1},{C_,MAJ,1},{BB,MAJ,1},{A_,MAJ,1}}},
  {"ArrMed",  8, {{A_,MIN,0},{G_,MAJ,0},{F_,MAJ,0},{E_,MAJ,0},{D_,MIN,1},{C_,MAJ,1},{BB,MAJ,1},{A_,MAJ,1}}},
  {"Mi-Fa",   4, {{F_,MAJ,0},{E_,MAJ,0},{F_,MAJ,0},{E_,MAJ,0}}},
  {"Solea",   8, {{E_,MAJ,0},{F_,MAJ,0},{E_,MAJ,0},{E_,MAJ,0},{A_,MIN,0},{G_,MAJ,0},{F_,MAJ,0},{E_,MAJ,0}}},
  {"Taranta", 4, {{B_,MIN,4},{A_,MAJ,4},{G_,MAJ,4},{FS,MAJ,4}}},
  {"Granain", 4, {{E_,MIN,5},{D_,MAJ,5},{C_,MAJ,5},{B_,MAJ,5}}},
  {"Alegria", 8, {{E_,MAJ,2},{A_,MAJ,2},{B_,DOM7,2},{E_,MAJ,2},{CS,MIN,2},{FS,MIN,2},{B_,DOM7,2},{E_,MAJ,2}}},
  {"Sevilla", 4, {{A_,MIN,3},{D_,MIN,3},{E_,DOM7,3},{A_,MIN,3}}},
};
// Andina: sanjuanito, yumbo, bomba, capishca (A menor / pentatonica)
static const ProgDef P_ANDI[] = {
  {"i-III",   4, {{A_,MIN,6},{C_,MAJ,6},{G_,MAJ,6},{A_,MIN,6}}},
  {"Sanjuan", 8, {{A_,MIN,6},{C_,MAJ,6},{G_,MAJ,6},{A_,MIN,6},{A_,MIN,6},{C_,MAJ,6},{E_,MIN,6},{A_,MIN,6}}},
  {"Bajada",  4, {{A_,MIN,3},{G_,MAJ,3},{F_,MAJ,3},{E_,DOM7,3}}},
  {"i-iv-V",  4, {{A_,MIN,3},{D_,MIN,3},{A_,MIN,3},{E_,DOM7,3}}},
  {"i-VI",    4, {{A_,MIN,9},{F_,MAJ,9},{C_,MAJ,9},{G_,MAJ,9}}},
  {"Yumbo",   4, {{A_,MIN,6},{G_,MAJ,6},{A_,MIN,6},{E_,MIN,6}}},
  {"Bomba",   4, {{A_,MIN,3},{E_,DOM7,3},{A_,MIN,3},{E_,DOM7,3}}},
  {"Bomba8",  8, {{A_,MIN,3},{D_,MIN,3},{E_,DOM7,3},{A_,MIN,3},{C_,MAJ,3},{G_,MAJ,3},{E_,DOM7,3},{A_,MIN,3}}},
  {"Capishc", 4, {{A_,MIN,3},{C_,MAJ,3},{E_,DOM7,3},{A_,MIN,3}}},
};
// Pasillo: E menor armonica, con su relativo mayor
static const ProgDef P_PASI[] = {
  {"i-iv-V",  4, {{E_,MIN,7},{A_,MIN,7},{B_,DOM7,7},{E_,MIN,7}}},
  {"Largo",   8, {{E_,MIN,7},{A_,MIN,7},{B_,DOM7,7},{E_,MIN,7},{G_,MAJ,7},{A_,MIN,7},{B_,DOM7,7},{E_,MIN,7}}},
  {"i-VI",    4, {{E_,MIN,7},{C_,MAJ,7},{A_,MIN,7},{B_,DOM7,7}}},
  {"V/iv",    4, {{E_,MIN,7},{E_,DOM7,7},{A_,MIN,7},{B_,DOM7,7}}},
  {"VII-III", 4, {{E_,MIN,7},{D_,MAJ,7},{G_,MAJ,7},{B_,DOM7,7}}},
  {"Relativ", 8, {{G_,MAJ,10},{D_,DOM7,10},{G_,MAJ,10},{B_,DOM7,7},{E_,MIN,7},{A_,MIN,7},{B_,DOM7,7},{E_,MIN,7}}},
  {"Quinto",  8, {{E_,MIN,7},{B_,DOM7,7},{E_,MIN,7},{E_,DOM7,7},{A_,MIN,7},{E_,MIN,7},{B_,DOM7,7},{E_,MIN,7}}},
};
// Bossa: C mayor con septimas; bossa menor en A
static const ProgDef P_BOSS[] = {
  {"I-VI-ii", 4, {{C_,MAJ7,8},{A_,DOM7,8},{D_,MIN7,8},{G_,DOM7,8}}},
  {"ii-V-I",  4, {{D_,MIN7,8},{G_,DOM7,8},{C_,MAJ7,8},{C_,MAJ7,8}}},
  {"Garota",  4, {{C_,MAJ7,8},{D_,DOM7,8},{D_,MIN7,8},{CS,DOM7,8}}},
  {"Backdor", 4, {{C_,MAJ7,8},{F_,MIN7,8},{BB,DOM7,8},{C_,MAJ7,8}}},
  {"Menor",   4, {{A_,MIN7,9},{D_,MIN7,9},{E_,DOM7,3},{A_,MIN7,9}}},
  {"Largo",   8, {{C_,MAJ7,8},{A_,DOM7,8},{D_,MIN7,8},{G_,DOM7,8},{E_,MIN7,8},{A_,DOM7,8},{D_,MIN7,8},{G_,DOM7,8}}},
  {"Cromat",  4, {{C_,MAJ7,8},{CS,DIM7,8},{D_,MIN7,8},{G_,DOM7,8}}},
  {"Ciclo",   8, {{E_,MIN7,8},{A_,DOM7,8},{D_,MIN7,8},{G_,DOM7,8},{C_,MAJ7,8},{F_,MAJ7,8},{B_,M7B5,8},{E_,DOM7,3}}},
};
// Bolero: C mayor y A menor, romantico
static const ProgDef P_BOLE[] = {
  {"I-vi",    4, {{C_,MAJ,8},{A_,MIN,8},{D_,MIN7,8},{G_,DOM7,8}}},
  {"Menor",   4, {{A_,MIN,3},{D_,MIN,3},{E_,DOM7,3},{A_,MIN,3}}},
  {"I-I7-iv", 4, {{C_,MAJ,8},{C_,DOM7,8},{F_,MAJ,8},{F_,MIN,8}}},
  {"Largo",   8, {{C_,MAJ7,8},{A_,MIN7,8},{D_,MIN7,8},{G_,DOM7,8},{E_,MIN7,8},{A_,DOM7,8},{D_,MIN7,8},{G_,DOM7,8}}},
  {"Ciclo",   8, {{A_,MIN7,10},{D_,DOM7,10},{G_,MAJ7,10},{C_,MAJ7,10},{FS,M7B5,7},{B_,DOM7,7},{E_,MIN,7},{E_,MIN,7}}},
  {"MenorLg", 8, {{A_,MIN,3},{D_,MIN7,3},{G_,DOM7,3},{C_,MAJ7,3},{F_,MAJ7,3},{B_,M7B5,3},{E_,DOM7,3},{A_,MIN,3}}},
  {"Trio",    8, {{C_,MAJ,8},{E_,DOM7,8},{A_,MIN,8},{C_,DOM7,8},{F_,MAJ,8},{F_,MIN,8},{G_,DOM7,8},{C_,MAJ,8}}},
};
// Electronica: A menor, pocos acordes (pedal, vaivenes), frigio para el acid, m7 para el dub
static const ProgDef P_ELEC[] = {
  {"Pedal",   4, {{A_,MIN,9},{A_,MIN,9},{A_,MIN,9},{A_,MIN,9}}},
  {"i-VII",   4, {{A_,MIN,9},{G_,MAJ,9},{A_,MIN,9},{G_,MAJ,9}}},
  {"i-VI",    4, {{A_,MIN,9},{F_,MAJ,9},{A_,MIN,9},{F_,MAJ,9}}},
  {"i-bII",   4, {{A_,MIN,1},{BB,MAJ,1},{A_,MIN,1},{BB,MAJ,1}}},
  {"i-iv",    4, {{A_,MIN,9},{D_,MIN,9},{A_,MIN,9},{D_,MIN,9}}},
  {"Dub",     4, {{A_,MIN7,9},{A_,MIN7,9},{D_,MIN7,9},{E_,MIN7,9}}},
  {"i-VI-VII",4, {{A_,MIN,9},{F_,MAJ,9},{G_,MAJ,9},{A_,MIN,9}}},
};
#undef E_
#undef F_
#undef FS
#undef G_
#undef GS
#undef A_
#undef BB
#undef B_
#undef C_
#undef CS
#undef D_
#undef EB

// random pools, relative to the mode tonic; index 5 is the tonic chord
struct PoolChord { int8_t rel; uint8_t q; };
static const PoolChord POOL_PHRYG[6] = {{5,MIN},{3,MAJ},{1,MAJ},{8,MAJ},{10,MIN},{0,MAJ}};
static const PoolChord POOL_JAZZ[6]  = {{5,MAJ7},{7,DOM7},{9,MIN7},{2,MIN7},{4,MIN7},{0,MAJ7}};
static const PoolChord POOL_MINOR[6] = {{5,MIN},{3,MAJ},{8,MAJ},{10,MAJ},{7,DOM7},{0,MIN}};
static const PoolChord POOL_PENTA[6] = {{3,MAJ},{10,MAJ},{7,MIN},{5,MIN},{8,MAJ},{0,MIN}};

enum Fam : uint8_t { H_AUTO = 0, H_FLAM, H_ANDI, H_PASI, H_BOSS, H_BOLE, H_LIBRE, H_ELEC, NUM_FAMS };   // H_ELEC after Libre: old presets keep their numbers
struct FamDef { const char *abbr; const char *name; const ProgDef *progs; uint8_t n; uint8_t mode; const PoolChord *pool; uint8_t cadential; };
static const FamDef FAMS[NUM_FAMS] = {
  {"Au", "Auto",     nullptr, 0, 0, nullptr, 0},
  {"Fl", "Flamenca", P_FLAM, sizeof(P_FLAM) / sizeof(ProgDef), M_E_PHR,  POOL_PHRYG, 2},
  {"An", "Andina",   P_ANDI, sizeof(P_ANDI) / sizeof(ProgDef), M_A_PENT, POOL_PENTA, 1},
  {"Pa", "Pasillo",  P_PASI, sizeof(P_PASI) / sizeof(ProgDef), M_E_HMIN, POOL_MINOR, 4},
  {"Bs", "Bossa",    P_BOSS, sizeof(P_BOSS) / sizeof(ProgDef), M_C_MAJ,  POOL_JAZZ,  1},
  {"Bo", "Bolero",   P_BOLE, sizeof(P_BOLE) / sizeof(ProgDef), M_C_MAJ,  POOL_JAZZ,  1},
  {"Li", "Libre",    nullptr, 0, 0, nullptr, 0},
  {"El", "Electronica", P_ELEC, sizeof(P_ELEC) / sizeof(ProgDef), M_A_NMIN, POOL_MINOR, 1},
};
static constexpr int MAX_PROGS = 10;   // largest family (Flamenca)

// ---------------------------------------------------------------- styles
enum StyleFlags : uint8_t { F_CELLS = 1, F_TERNARY = 2, F_STRUM = 4, F_ELEC = 8, F_ACID = 16, F_LINE16 = 32 };   // LINE16: the melody is a semicorchea line
struct StyleDef {
  const char *name;
  uint8_t len;        // cycle length in corcheas
  uint8_t beat;       // corcheas per displayed beat
  uint8_t start;      // count shown at the first beat
  uint64_t a16;       // main hits (output A), bit = semicorchea of the cycle
  uint64_t pref16;    // characteristic secondary hits (output B, comping)
  uint8_t remate16;
  uint8_t flags;
  uint8_t fam, prog;  // harmony used by Auto
  uint8_t orn;        // melodic ornament family (index into FAMS)
};
static constexpr uint64_t Counts(uint32_t counts, int start, int len) {
  uint64_t m = 0;
  for (int p = 0; p < len; ++p) { const int c = ((start - 1 + p) % len) + 1; if (counts & (1u << (c - 1))) m |= 1ull << (4 * p); }
  return m;
}
static constexpr int CountPos(int count, int start, int len) { return 4 * (((count - start) % len + len) % len); }
template <typename... T> static constexpr uint64_t POS(T... p) { return (0ull | ... | (1ull << p)); }
#define K(c) (1u << ((c) - 1))
enum StyleId { S_BULERIA = 0, S_SOLEA, S_ALEGRIAS, S_SEGUIRIYA, S_TANGOS, S_SEVILLANAS,
               S_SANJUANITO, S_PASILLO, S_BOMBA, S_YUMBO, S_CAPISHCA, S_BOSSA, S_BOLERO,
               S_TECHNO, S_ACID, S_MINIMAL, S_DUB, S_ELECTRO, NUM_STYLES };
static const StyleDef STYLES[NUM_STYLES] = {
  {"Bulerias",   24, 2, 12, Counts(K(12)|K(3)|K(6)|K(8)|K(10), 12, 12), 0, CountPos(10, 12, 12), F_CELLS|F_TERNARY|F_STRUM, H_FLAM, 0, H_FLAM},
  {"Solea",      24, 2,  1, Counts(K(3)|K(6)|K(8)|K(10)|K(12),  1, 12), 0, CountPos(10,  1, 12), F_CELLS|F_TERNARY|F_STRUM, H_FLAM, 5, H_FLAM},
  {"Alegrias",   24, 2, 12, Counts(K(3)|K(6)|K(8)|K(10)|K(12), 12, 12), 0, CountPos(10, 12, 12), F_CELLS|F_TERNARY|F_STRUM, H_FLAM, 8, H_FLAM},
  {"Seguiriya",  24, 2,  1, Counts(K(1)|K(3)|K(5)|K(8)|K(11),   1, 12), 0, CountPos(11,  1, 12), F_CELLS|F_TERNARY|F_STRUM, H_FLAM, 2, H_FLAM},
  {"Tangos",     16, 2,  1, Counts(K(2)|K(3)|K(4)|K(6)|K(7)|K(8), 1, 8), 0, CountPos(7, 1, 8),  F_CELLS|F_STRUM, H_FLAM, 0, H_FLAM},
  {"Sevillanas", 24, 2,  1, Counts(K(1)|K(4)|K(7)|K(10),         1, 12), 0, CountPos(10,  1, 12), F_CELLS|F_TERNARY|F_STRUM, H_FLAM, 9, H_FLAM},
  {"Sanjuanito",  8, 2, 1, POS(0, 4, 8, 12),   POS(2, 3, 6, 7, 10, 11, 14, 15), 12, F_STRUM, H_ANDI, 1, H_ANDI},
  {"Pasillo",    12, 2, 1, POS(0, 12),         POS(6, 8, 18, 20),               20, F_STRUM, H_PASI, 1, H_PASI},
  {"Bomba",      12, 3, 1, POS(4, 10, 16, 22), POS(0, 2, 3, 6, 8, 9, 12, 14, 15, 18, 20, 21), 22, F_TERNARY|F_STRUM, H_ANDI, 6, H_ANDI},
  {"Yumbo",      12, 3, 1, POS(0, 6, 12, 18),  POS(2, 8, 14, 20),               18, F_TERNARY|F_STRUM, H_ANDI, 5, H_ANDI},
  {"Capishca",   12, 3, 1, POS(0, 6, 12, 18),  POS(4, 10, 16, 22),              22, F_TERNARY|F_STRUM, H_ANDI, 8, H_ANDI},
  {"Bossa",       8, 2, 1, POS(0, 6, 8, 14),   POS(3, 10, 13),                  14, 0, H_BOSS, 0, H_BOSS},
  {"Bolero",     16, 2, 1, POS(0, 8, 16, 24),  POS(4, 6, 12, 14, 20, 22, 28, 30), 28, F_STRUM, H_BOLE, 0, H_BOLE},
  // electronic, one 4/4 bar of 16 semicorcheas. A = kick, B = hats / percussion, no remates
  {"Techno",      8, 2, 1, POS(0, 4, 8, 12),   POS(2, 6, 10, 14),               14, F_ELEC|F_LINE16, H_ELEC, 1, H_ELEC},
  {"Acid",        8, 2, 1, POS(0, 4, 8, 12),   POS(2, 6, 10, 14),               14, F_ELEC|F_ACID|F_LINE16, H_ELEC, 3, H_ELEC},
  {"Minimal",     8, 2, 1, POS(0, 4, 8, 12),   POS(3, 7, 11, 14),               14, F_ELEC|F_LINE16, H_ELEC, 0, H_ELEC},
  {"Dub techno",  8, 2, 1, POS(0, 4, 8, 12),   POS(3, 6, 10, 14),               14, F_ELEC|F_LINE16, H_ELEC, 5, H_ELEC},
  {"Electro",     8, 2, 1, POS(0, 7, 10),      POS(4, 12),                      12, F_ELEC|F_LINE16, H_ELEC, 2, H_ELEC},
};

// how the melody moves in each style. Weights of each step (in scale degrees), the register it
// leans towards, how much it lands on chord tones and how much it repeats its own figure.
struct MelProfile {
  uint8_t rep, dn1, up1, dn2, up2, leap, oct;   // weights: repeat, -1, +1, -2, +2, a 4th/5th, octave
  uint8_t center;    // preferred position, in % of Rango
  uint8_t chord;     // % of accents that land on a chord tone (100 = always, every note)
  uint8_t motif;     // % of beats that repeat the figure of the previous beat
  uint8_t rate;      // how often the melody plays on the rhythm's figure, % of the usual
};
static const MelProfile MELP[NUM_STYLES] = {
  // flamenco: stepwise and falling (cadencia andaluza), lots of repeated notes, narrow
  {25, 38, 18, 9, 6, 3, 1,  45, 70, 15, 100},   // Bulerias
  {28, 40, 15, 9, 5, 2, 1,  40, 75, 15, 100},   // Solea (more grave, more falling)
  {20, 28, 28, 8, 9, 6, 1,  55, 70, 20, 110},   // Alegrias (major, livelier, rises more)
  {30, 40, 12, 10, 4, 3, 1, 35, 80, 10, 90},   // Seguiriya (grave, lament)
  {22, 32, 24, 9, 8, 4, 1,  50, 70, 25, 110},   // Tangos
  {18, 26, 26, 8, 8, 12, 2, 55, 85, 35, 110},   // Sevillanas (sung couplets: leaps and repeats)
  // andean: pentatonic, 4ths and 5ths, short figures that repeat
  {22, 24, 20, 12, 10, 10, 2, 50, 75, 45, 170}, // Sanjuanito
  {12, 28, 18, 8, 10, 20, 4, 60, 90, 20, 110},  // Pasillo (lyrical, wide, arpeggios then steps down)
  {24, 22, 22, 12, 10, 8, 2, 45, 75, 40, 150},  // Bomba
  {28, 24, 20, 12, 8, 6, 2,  40, 70, 50, 140},  // Yumbo (hypnotic, repetitive)
  {22, 22, 22, 12, 12, 8, 2, 50, 75, 45, 150},  // Capishca
  {18, 32, 30, 7, 7, 5, 1,  55, 60, 20, 120},   // Bossa (smooth, stepwise, tensions)
  {12, 30, 20, 8, 10, 16, 4, 60, 90, 15, 100},  // Bolero (romantic leaps, resolves down)
  // electronic
  {45, 17, 17, 6, 6, 4, 8,  25, 85, 60, 100},   // Techno: riff
  {30, 15, 15, 8, 8, 8, 16, 20, 70, 40, 100},   // Acid: low 303 line with octave jumps
  {60, 10, 10, 4, 4, 6, 6,  20, 95, 75, 100},   // Minimal: almost a pedal, root and fifth
  {35, 12, 12, 10, 10, 15, 6, 35, 100, 60},// Dub techno: only chord tones
  {30, 14, 14, 6, 6, 14, 16, 25, 85, 50, 100},  // Electro: octaves and fifths
};
#undef K

static inline uint32_t hash(uint32_t a, uint32_t b, uint32_t c) {
  uint32_t x = a * 0x9E3779B1u ^ b * 0x85EBCA77u ^ c * 0xC2B2AE3Du;
  x ^= x >> 15; x *= 0x2C1B3C6Du; x ^= x >> 12; x *= 0x297A2D39u; x ^= x >> 15;
  return x;
}
static inline int rnd100(uint32_t a, uint32_t b, uint32_t c) { return (int)(hash(a, b, c) % 100); }

// ---------------------------------------------------------------- events
enum Voice : uint8_t { V_CLARA = 0, V_SORDA, V_PIE_GRAVE, V_PIE_AGUDO, NUM_EXTRA_VOICES };
struct Event {
  uint8_t fine_a = 0;                 // output A / MIDI grave
  uint8_t vel_a = 110;
  uint8_t fine_b = 0;                 // output B / MIDI agudo
  uint8_t vel_b[FINE] = {0};
  uint8_t vmask[NUM_EXTRA_VOICES] = {0};   // extra MIDI voices: palmas, pies (bits 0 or 3)
  uint8_t vvel[NUM_EXTRA_VOICES] = {0};
  uint8_t comp = 0;                   // chord hits (bits 0 or 3)
  uint8_t comp_vel = 80;
  bool strum = false;
  uint8_t push = 0;
  uint8_t mel_n = 0;                  // melody changes in this corchea
  uint8_t mel_main = 0;               // which of them is the real note (the rest are ornaments)
  uint8_t mel_at[4] = {0};
  int8_t mel_semis[4] = {0};
  uint8_t mel_notes = 0;              // entries that are real notes (bit i = entry i): CV + gate
  uint8_t mel_acc = 0;                // accented notes (acid)
  uint8_t mel_tie = 0;                // notes tied into the next one: the next note slides (acid)
  uint8_t bass_n = 0;                 // electronic styles: the bass line of this corchea (semicorchea 0 / 3)
  uint8_t bass_at[2] = {0, 0};
  int8_t bass_semis[2] = {0, 0};
  uint8_t bass_acc = 0;
  bool new_chord = false;
  bool remate = false;
  bool beat_start = false;            // first corchea of a beat
  bool cycle_start = false;
};

static constexpr int NUM_LOOPS = 8;
static const char *const LOOP_NAMES[NUM_LOOPS] = {
  "1 tiempo", "2 tiempos", "4 tiempos", "Medio ciclo", "1 ciclo", "2 ciclos", "4 ciclos", "8 ciclos"};
static const char *const HLOOP_NAMES[4] = {"1 acorde", "2 acordes", "4 acordes", "Frase"};

static const uint8_t CELLS_2[] = {0x00, 0x02};
static const uint8_t CELLS_4[] = {0x00, 0x04, 0x08, 0x0C, 0x0A, 0x0E};
static const uint8_t CELLS_6[] = {0x00, 0x04, 0x10, 0x08, 0x14, 0x28, 0x34, 0x2A, 0x3E};
static inline int Popcount(uint32_t b) { int n = 0; while (b) { n += b & 1; b >>= 1; } return n; }

enum Lock : uint8_t { L_NADA = 0, L_RITMO, L_MELODIA, L_ARMONIA, L_RIT_MEL, L_TODO, NUM_LOCKS };

class Engine {
public:
  // parameters
  int style = 0, fam = H_AUTO, prog = 1;   // prog 0 = Azar, 1.. = fixed progressions
  int reharm = 0;                          // 0..3
  int density = 55, soltura = 15, adorno = 30, empuje = 20;
  int frase = 4, range = 10, lock = L_NADA;
  int loop_r = 4, loop_m = 5, loop_h = 3;   // rhythm/melody: LOOP_NAMES index; harmony: 1,2,4 chords or phrase
  int mel_style = 0;      // 0 Clasica (the 4.1 melody), 1 Ornamentada (4.4, fixed: note on the beat)
  int notas = 50;         // how many notes the melody plays; above 50 it also plays in the gaps
  int perfil = 0;         // 0 Tradicional, 1 Techno (straight grid, constant palmas/pies, riff melody)
  bool remates = true;    // natural remates at the end of the harmony loop

  // state
  int active_style = 0;
  int tick = -1, cycle = -1;
  bool remate_cycle = false;
  int melody_semis = 12;
  int active_fam = H_FLAM, active_prog = 1;     // resolved selection
  int req_fam = -1, req_prog = -1;              // selection the phrase was built from
  int phrase_len = 4, chord_idx = 0;
  Chord phrase[8] = {{5,MIN,0},{3,MAJ,0},{1,MAJ,0},{0,MAJ,0},{5,MIN,0},{3,MAJ,0},{1,MAJ,0},{0,MAJ,0}};

  // the material: every seed of every layer. With Soltura 0 the same material plays the same
  // rhythm, melody and harmony again.
  struct Material {
    uint32_t r_seed[MAX_LOOP], m_seed[MAX_LOOP], h_seed, entropy;
    int8_t m_pos[MAX_LOOP];
    uint8_t r_valid, m_valid, h_valid, pos;
  };
  void GetMaterial(Material &m) const {
    for (int i = 0; i < MAX_LOOP; ++i) { m.r_seed[i] = r_seed[i]; m.m_seed[i] = m_seed[i]; m.m_pos[i] = m_pos[i]; }
    m.h_seed = h_seed; m.entropy = entropy; m.r_valid = r_valid; m.m_valid = m_valid; m.h_valid = h_valid; m.pos = (uint8_t)pos;
  }
  // loads the material and restarts the loops and the phrase from the top at the next tick
  void SetMaterial(const Material &m) {
    for (int i = 0; i < MAX_LOOP; ++i) { r_seed[i] = m.r_seed[i]; m_seed[i] = m.m_seed[i]; m_pos[i] = m.m_pos[i]; }
    h_seed = m.h_seed; entropy = m.entropy ? m.entropy : 1; r_valid = m.r_valid; m_valid = m.m_valid; h_valid = m.h_valid;
    pos = m.pos; req_fam = -1; chord_count = 0; chord_idx = 0; remate_req = false;
    tick = -1; g = -1; cycle = 0; active_style = style;
  }
  void Seed(uint32_t s) { entropy = s ? s : 1; }
  // fresh material that depends only on s: the same s always gives the same rhythm, melody and harmony
  void Restart(uint32_t s) { Seed(s); r_valid = m_valid = 0; h_valid = false; req_fam = -1; tick = -1; g = -1; cycle = -1; chord_count = 0; pos = 7; }

  void Reset() { tick = -1; cycle = -1; remate_req = false; chord_idx = 0; chord_count = 0; req_fam = -1; g = -1; r_valid = m_valid = 0; }
  int LoopTicks(int opt) const {
    const StyleDef &d = Def();
    switch (opt) {
      case 0: return d.beat; case 1: return 2 * d.beat; case 2: return 4 * d.beat; case 3: return d.len / 2;
      case 4: return d.len;  case 5: return 2 * d.len;  case 6: return 4 * d.len;  default: return 8 * d.len;
    }
  }
  void Realign() { if (tick > 0) { tick = -1; g = (g / Def().len + 1) * Def().len - 1; } }
  void RequestRemate() { remate_req = true; }
  // new material for every layer that is not frozen
  void Reroll() {
    for (int i = 0; i < MAX_LOOP; ++i) {
      if (!LockedR()) r_seed[i] = Fresh();
      if (!LockedM()) { m_seed[i] = Fresh(); m_pos[i] = (int8_t)pos; }
    }
    if (!LockedH()) { h_seed = Fresh(); req_fam = -1; }
  }

  const StyleDef &Def() const { return STYLES[active_style]; }
  int NumBeats() const { return Def().len / Def().beat; }
  int Beat() const { return tick < 0 ? 0 : tick / Def().beat; }
  int CountAt(int b) const { return ((Def().start - 1 + b) % NumBeats()) + 1; }
  bool BeatHasA(int b) const {
    const StyleDef &d = Def();
    for (int i = b * d.beat * 2; i < (b + 1) * d.beat * 2; ++i) if (d.a16 & (1ull << i)) return true;
    return false;
  }
  const Chord &CurChord() const { return phrase[chord_idx]; }
  const Mode &CurMode() const { return MODES[CurChord().m]; }
  int RootSemis() const { return CurChord().root; }
  // what the display should show for the selection (the pending one if not applied yet)
  int ShownFam() const { return fam == H_AUTO ? STYLES[style].fam : fam; }
  int ShownProg() const { return fam == H_AUTO ? STYLES[style].prog + 1 : prog; }
  bool SelectionPending() const { return fam != req_fam || prog != req_prog || style != active_style; }
  static int NumProgs(int f) { return (f == H_AUTO || f == H_LIBRE) ? 1 : FAMS[f].n + 1; }
  static const char *ProgName(int f, int p) {
    if (f == H_AUTO) return "Auto";
    if (f == H_LIBRE || p <= 0) return "Azar";
    return FAMS[f].progs[p - 1].name;
  }

  void ChordName(char *out) const {
    const Chord &c = CurChord(); const char *n = NOTE_NAMES[((c.root % 12) + 12) % 12];
    int i = 0; while (*n) out[i++] = *n++;
    const char *suf = "";
    switch (c.q) { case MIN: suf = "m"; break; case DOM7: suf = "7"; break; case MAJ7: suf = "M7"; break;
                   case MIN7: suf = "m7"; break; case M7B5: suf = "m7b5"; break; case DIM7: suf = "dim"; break; }
    while (*suf) out[i++] = *suf++;
    out[i] = 0;
  }
  int ChordTones(int8_t *iv) const {
    const Chord &c = CurChord();
    iv[0] = 0;
    switch (c.q) {
      case MIN:  iv[1] = 3; iv[2] = 7; return 3;
      case DOM7: iv[1] = 4; iv[2] = 7; iv[3] = 10; return 4;
      case MAJ7: iv[1] = 4; iv[2] = 7; iv[3] = 11; return 4;
      case MIN7: iv[1] = 3; iv[2] = 7; iv[3] = 10; return 4;
      case M7B5: iv[1] = 3; iv[2] = 6; iv[3] = 10; return 4;
      case DIM7: iv[1] = 3; iv[2] = 6; iv[3] = 9; return 4;
      default:   iv[1] = 4; iv[2] = 7; return 3;
    }
  }
  // note for a Just Friends in PLUME with Intone fully left (undertones build a minor triad
  // whose fifth is the note sent)
  int JFSemis() const {
    const Chord &c = CurChord();
    int t;
    if (c.q == MIN || c.q == MIN7) t = c.root - 5;
    else if (c.q == DOM7) t = c.root + 2;
    else if (c.q == M7B5 || c.q == DIM7) t = c.root + 1;   // (X+3)m-ish over the diminished colour
    else t = c.root - 1;
    return ((t % 12) + 12) % 12;
  }

  Event Tick() {
    Event ev;
    tick++;
    if (tick < 0 || tick >= Def().len) tick = 0;
    if (tick == 0) { StartCycle(); ev.new_chord = true; }

    const StyleDef &d = Def();
    g++;
    ev.cycle_start = (tick == 0);
    ev.beat_start = (tick % d.beat) == 0;
    // rhythm and melody loops, each with its own length; a loop longer than a cycle keeps
    // one seed per cycle (Turing style), a shorter one repeats inside the cycle
    const int rseg = LayerStep(LoopTicks(loop_r), r_seed, r_valid, nullptr, LockedR());
    const int mseg = LayerStep(LoopTicks(loop_m), m_seed, m_valid, m_pos, LockedM());
    rp = g % LoopTicks(loop_r);
    mp = g % LoopTicks(loop_m);
    rkey = r_seed[rseg] ^ (remate_cycle ? 0x5EA1u : 0u);
    mkey = m_seed[mseg];
    bkey = hash(mkey, 0xBA55u, 0);
    const int rtick = d.remate16 / 2;
    if (remate_req && !remate_cycle && tick <= rtick) { remate_cycle = true; remate_req = false; }
    if (remate_cycle && tick > rtick) return ev;

    const int s0 = tick * 2;
    const bool a_on = d.a16 & (1ull << s0), a_off = d.a16 & (1ull << (s0 + 1));
    if (a_on) ev.fine_a |= 1u;
    if (a_off) ev.fine_a |= 1u << 3;
    ev.vel_a = (tick == 0) ? 120 : 105;
    if (remate_cycle && (s0 == d.remate16 || s0 + 1 == d.remate16)) { ev.remate = true; ev.vel_a = 127; }

    // short rhythm loops decide fills hit by hit inside the loop, so they repeat like a pattern
    const bool short_r = LoopTicks(loop_r) < d.len;
    bool base = (d.flags & F_ELEC) ? ElecTick(ev, s0) : ((d.flags & F_CELLS) && !short_r) ? CellsTick(ev, a_on) : PrefTick(ev, s0);
    const int nt = (tick + 1) % d.len;
    const bool next_a = d.a16 & (1ull << (nt * 2));
    if (!ev.fine_b && next_a && rnd100(rkey ^ 0xA7C1u, rp, 4) < adorno * 6 / 10) {
      const int k = rnd100(rkey ^ 0x1234u, rp, 5) < 60 ? 3 : 4;
      ev.fine_b |= 1u << k; ev.vel_b[k] = 95;
    }
    if (remate_cycle && tick >= rtick - 3 && rnd100(rkey ^ 0x4ED0u, rp, 6) < 40 + adorno / 2) {
      ev.fine_b |= 1u | (1u << 3); ev.vel_b[0] = 90; ev.vel_b[3] = 100;
    }
    if (ev.fine_b && empuje > 0 && !Techno()) ev.push = (uint8_t)(rnd100(rkey ^ 0x3131u, rp, 7) * empuje * 15 / 10000);

    ExtraVoices(ev, s0, next_a);
    Comping(ev, s0, a_on || a_off);

    if (Acid()) { AcidLine(ev, s0); return ev; }
    if (Def().flags & F_LINE16) { BassLine(ev, s0); LeadLine(ev, s0); return ev; }
    bool mel = a_on || a_off;
    if (!mel && base) mel = rnd100(mkey ^ 0xABCDu, mp, 8) < (notas < 50 ? notas * 70 / 100 : 35 + (notas - 50) * 13 / 10) * MP().rate / 100;
    if (!mel && !base && notas > 50) mel = rnd100(mkey ^ 0xABCEu, mp, 9) < (notas - 50) * 8 / 10;
    if (mel) Melody(ev, a_on || a_off);
    return ev;
  }

private:
  int pos = 7;
  bool remate_req = false;
  uint32_t r_seed[MAX_LOOP] = {0}, m_seed[MAX_LOOP] = {0}, h_seed = 0;
  int8_t m_pos[MAX_LOOP] = {0};
  uint8_t r_valid = 0, m_valid = 0;
  bool h_valid = false;
  uint32_t entropy = 1, rkey = 0, mkey = 0, bkey = 0;
  int32_t g = -1;          // corcheas since the start
  int rp = 0, mp = 0;      // position inside the rhythm / melody loop
  int chord_count = 0;

  // one layer of loop memory: returns the segment in use, mutating it at its start
  int LayerStep(int L, uint32_t *seeds, uint8_t &valid, int8_t *posmem, bool locked) {
    const int len = Def().len;
    const int seg_len = L <= len ? L : len;
    int nseg = L / seg_len; if (nseg > MAX_LOOP) nseg = MAX_LOOP;
    const int seg = (int)((g / seg_len) % nseg);
    if (g % seg_len == 0) {
      const uint8_t bit = 1u << seg;
      const bool fresh = !(valid & bit);
      if (fresh || (!locked && (int)(Fresh() % 100) < soltura)) {
        seeds[seg] = Fresh();
        if (posmem) posmem[seg] = (int8_t)pos;
        valid |= bit;
      } else if (posmem) {
        pos = posmem[seg];                 // replay: the phrase starts where it did
      }
    }
    return seg;
  }
  int HLoop() const {
    static const int8_t n[4] = {1, 2, 4, 8};
    const int h = loop_h >= 3 ? phrase_len : n[loop_h];
    return h < phrase_len ? h : phrase_len;
  }

  bool Techno() const { return perfil == 1 || (Def().flags & F_ELEC); }
  bool Acid() const { return Def().flags & F_ACID; }
  const MelProfile &MP() const { return MELP[active_style]; }
  bool LockedR() const { return lock == L_RITMO || lock == L_RIT_MEL || lock == L_TODO; }
  bool LockedM() const { return lock == L_MELODIA || lock == L_RIT_MEL || lock == L_TODO; }
  bool LockedH() const { return lock == L_ARMONIA || lock == L_TODO; }
  uint32_t Fresh() { entropy = hash(entropy, 0x51CEu, (uint32_t)cycle); return entropy; }

  // ------------------------------------------------ flamenco cells
  uint64_t AccTicks() const {
    uint64_t m = 0; const StyleDef &d = Def();
    for (int t = 0; t < d.len; ++t) if (d.a16 & (1ull << (2 * t))) m |= 1ull << t;
    return m;
  }
  int NumAccents() const { const uint64_t m = AccTicks(); return Popcount((uint32_t)m) + Popcount((uint32_t)(m >> 32)); }
  int AccentTick(int k) const {
    const uint64_t m = AccTicks(); int n = 0;
    for (int t = 0; t < Def().len; ++t) if (m & (1ull << t)) { if (n == k) return t; n++; }
    return 0;
  }
  int GroupOf(int t) const { const int n = NumAccents(); for (int k = n - 1; k >= 0; --k) if (t >= AccentTick(k)) return k; return n - 1; }
  int GroupLen(int g) const {
    const int n = NumAccents(), L = Def().len;
    return ((g + 1 < n) ? AccentTick(g + 1) : AccentTick(0) + L) - AccentTick(g);
  }
  uint8_t CellFor(int g) const {
    const int L = GroupLen(g);
    const uint8_t *cells; int nc;
    if (L <= 2) { cells = CELLS_2; nc = sizeof(CELLS_2); }
    else if (L <= 4) { cells = CELLS_4; nc = sizeof(CELLS_4); }
    else { cells = CELLS_6; nc = sizeof(CELLS_6); }
    int dens = density;
    if (remate_cycle) dens += 45 * AccentTick(g) / (Def().remate16 / 2 + 1);
    else if (g == NumAccents() - 1 && rnd100(rkey ^ 0x4444u, g, 0) < 35) dens += 40;
    if (dens > 100) dens = 100;
    int weights[9], total = 0;
    for (int i = 0; i < nc; ++i) {
      const int full = Popcount(cells[i]) * 100 / (L > 1 ? L - 1 : 1);
      const int diff = full > dens ? full - dens : dens - full;
      weights[i] = diff < 100 ? (100 - diff) * (100 - diff) / 100 + 1 : 1;
      total += weights[i];
    }
    int r = (int)(hash(rkey ^ 0xCE11u, g, 0) % (uint32_t)total);
    for (int i = 0; i < nc; ++i) { if (r < weights[i]) return cells[i] & ((1u << L) - 1); r -= weights[i]; }
    return 0;
  }
  bool CellsTick(Event &ev, bool a_on) {
    if (a_on) {
      if (rnd100(rkey ^ 0xB0B0u, rp, 1) < adorno * 25 / 100) { ev.fine_b |= 1u << 3; ev.vel_b[3] = 55; }
      return false;
    }
    const int g = GroupOf(tick);
    int off = tick - AccentTick(g); if (off < 0) off += Def().len;
    const bool base = CellFor(g) & (1u << off);
    if (base) { ev.fine_b |= 1u; ev.vel_b[0] = 80 + rnd100(rkey ^ 0x0F0Fu, rp, 2) / 5; Doubling(ev); }
    return base;
  }
  // electronic styles: output B is a steady hat (or the snare in Electro); Densidad adds 16th ghost hits
  bool ElecTick(Event &ev, int s0) {
    const StyleDef &d = Def();
    const bool electro = active_style == S_ELECTRO;
    bool base = false;
    for (int h = 0; h < 2; ++h) {
      const int s = s0 + h, pos = s & 15;
      if (d.a16 & (1ull << s)) continue;
      const bool main = electro ? (d.pref16 & (1ull << s)) != 0 : (pos & 3) == 2;
      int chance = main ? (density >= 10 ? 100 : 0) : (pos & 1) ? density * 6 / 10 - 10 : density / 3 - 5;
      if (electro && !main) chance = (pos & 1) ? density / 2 - 5 : density / 4;
      if (rnd100(rkey ^ 0xE1E1u, rp * 2 + h, 1) < chance) {
        const int k = h * 3;
        ev.fine_b |= 1u << k;
        ev.vel_b[k] = main ? 100 : 45 + rnd100(rkey, rp * 2 + h, 3) / 4;
        if (h == 0) base = true;
      }
    }
    return base;
  }
  bool PrefTick(Event &ev, int s0) {
    const StyleDef &d = Def();
    bool base = false;
    for (int h = 0; h < 2; ++h) {
      const int s = s0 + h;
      if (d.a16 & (1ull << s)) continue;
      const bool pref = d.pref16 & (1ull << s);
      int chance = pref ? 40 + density * 70 / 100 : density * (s % 4 == 0 ? 35 : (s % 2 == 0 ? 30 : 10)) / 100;
      if (remate_cycle) chance += 30 * tick / (d.remate16 / 2 + 1);
      if (rnd100(rkey ^ 0xF1F1u, (rp * 2 + h), 1) < chance) {
        const int k = h * 3;
        ev.fine_b |= 1u << k;
        ev.vel_b[k] = pref ? 85 + rnd100(rkey, (rp * 2 + h), 2) / 7 : 45 + rnd100(rkey, (rp * 2 + h), 3) / 4;
        if (h == 0) base = true;
      }
    }
    if (base) Doubling(ev);
    return base;
  }
  void Doubling(Event &ev) {
    if (rnd100(rkey ^ 0xD0B1u, rp, 2) >= adorno / 2) return;
    if ((Def().flags & F_TERNARY) && !Techno() && rnd100(rkey ^ 0x7E51u, rp, 3) < 40) {
      ev.fine_b |= (1u << 2) | (1u << 4); ev.vel_b[2] = 60; ev.vel_b[4] = 70;
    } else if (!(ev.fine_b & (1u << 3))) {
      ev.fine_b |= 1u << 3; ev.vel_b[3] = 55;
    }
  }

  // ------------------------------------------------ palmas y pies (MIDI only)
  void ExtraVoices(Event &ev, int s0, bool next_a) {
    const StyleDef &d = Def();
    if (Techno() && active_style != S_ELECTRO) {        // pie grave every beat, palma clara on backbeats, palma sorda on offbeats
      const int beat16 = d.beat * 2;
      for (int h = 0; h < 2; ++h) {
        const int s = s0 + h, bit = h * 3, in_beat = s % beat16, b = s / beat16;
        if (in_beat == 0) {
          ev.vmask[V_PIE_GRAVE] |= 1u << bit; ev.vvel[V_PIE_GRAVE] = 110;
          if (b & 1) { ev.vmask[V_CLARA] |= 1u << bit; ev.vvel[V_CLARA] = 105; }
        } else if (in_beat == beat16 / 2) {
          ev.vmask[V_SORDA] |= 1u << bit; ev.vvel[V_SORDA] = 70 + rnd100(rkey, (rp * 2 + h), 9) / 5;
        } else if (rnd100(rkey ^ 0x7AC0u, (rp * 2 + h), 4) < density * adorno / 250) {
          ev.vmask[V_PIE_AGUDO] |= 1u << bit; ev.vvel[V_PIE_AGUDO] = 55 + rnd100(rkey, (rp * 2 + h), 5) / 3;
        }
      }
      (void)next_a;
      return;
    }
    const int beat16 = d.beat * 2;
    const bool flam = d.flags & F_CELLS;
    for (int h = 0; h < 2; ++h) {
      const int s = s0 + h, bit = h * 3;
      const bool isA = d.a16 & (1ull << s), pref = d.pref16 & (1ull << s);
      const int inBeat = s % beat16;
      const bool onBeat = inBeat == 0;
      // palma clara: the accents in flamenco, the characteristic figure elsewhere
      if ((flam && isA) || (!flam && pref && rnd100(rkey ^ 0xC1A0u, (rp * 2 + h), 1) < 35 + density / 2)) {
        ev.vmask[V_CLARA] |= 1u << bit; ev.vvel[V_CLARA] = isA ? 110 : 85;
      }
      // palma sorda: contratiempo
      const bool contra = (d.beat == 3) ? (inBeat == 2 || inBeat == 4) : (inBeat == 2);
      if (contra && !isA && rnd100(rkey ^ 0x50DAu, (rp * 2 + h), 2) < 30 + density * 6 / 10) {
        ev.vmask[V_SORDA] |= 1u << bit; ev.vvel[V_SORDA] = 55 + rnd100(rkey, (rp * 2 + h), 9) / 5;
      }
      // pie grave (planta): the pulse
      if (onBeat && rnd100(rkey ^ 0x91E0u, (rp * 2 + h), 3) < 55 + density / 3) {
        ev.vmask[V_PIE_GRAVE] |= 1u << bit; ev.vvel[V_PIE_GRAVE] = isA ? 105 : 75;
      }
      // pie agudo (tacon): zapateado runs, more before accents and into the remate
      int z = density * adorno / 150;
      if (next_a && h == 1) z += 30;
      if (remate_cycle) z += 35 * tick / (d.remate16 / 2 + 1);
      if (!onBeat && rnd100(rkey ^ 0x7AC0u, (rp * 2 + h), 4) < z) {
        ev.vmask[V_PIE_AGUDO] |= 1u << bit; ev.vvel[V_PIE_AGUDO] = 60 + rnd100(rkey, (rp * 2 + h), 5) / 3;
      }
    }
  }

  // ------------------------------------------------ chord rhythm (MIDI only)
  void Comping(Event &ev, int s0, bool a_hit) {
    const StyleDef &d = Def();
    ev.strum = d.flags & F_STRUM;
    if (d.flags & F_CELLS) {                  // rasgueo on the accents, some fills
      if (a_hit) { ev.comp |= 1u; ev.comp_vel = 105; }
      else if ((ev.fine_b & 1u) && rnd100(rkey ^ 0xCA11u, rp, 1) < 40 + density / 3) { ev.comp |= 1u; ev.comp_vel = 75; }
      return;
    }
    for (int h = 0; h < 2; ++h) {
      const int s = s0 + h;
      const bool pref = d.pref16 & (1ull << s);
      if ((s == 0) || (pref && rnd100(rkey ^ 0xC0C0u, (rp * 2 + h), 1) < 55 + density * 4 / 10)) {
        ev.comp |= 1u << (h * 3); ev.comp_vel = s == 0 ? 95 : 78;
      }
    }
    (void)a_hit;
  }

  // ------------------------------------------------ harmony
  void StartCycle() {
    if (active_style != style) { active_style = style; cycle = 0; g = -1; r_valid = m_valid = 0; }
    else cycle++;

    const bool changed = (fam != req_fam || prog != req_prog);
    if (cycle == 0 || changed) { chord_count = 0; BuildPhrase(false); }
    else if (++chord_count >= HLoop()) {
      // the harmony loop wraps: Turing-style chance of rewriting it
      chord_count = 0;
      const bool mutate = !LockedH() && (int)(Fresh() % 100) < soltura;
      if (mutate || HLoop() == phrase_len) BuildPhrase(mutate);
    }
    chord_idx = chord_count < phrase_len ? chord_count : 0;
    remate_cycle = (remates && !Techno() && chord_count == HLoop() - 1) || remate_req;
    remate_req = false;
  }

  void BuildPhrase(bool mutate) {
    req_fam = fam; req_prog = prog;
    int f = fam, p = prog;
    if (f == H_AUTO) { f = Def().fam; p = Def().prog + 1; }
    if (!h_valid) { h_seed = Fresh(); h_valid = true; }
    else if (mutate) h_seed = Fresh();
    const uint32_t seed = h_seed;
    if (f == H_LIBRE) { f = 1 + (int)(hash(seed, 7, 7) % 5); p = 0; }
    active_fam = f; active_prog = p;
    const FamDef &fd = FAMS[f];
    if (p > 0 && p <= fd.n) {
      const ProgDef &pd = fd.progs[p - 1];
      phrase_len = pd.len;
      for (int i = 0; i < pd.len; ++i) phrase[i] = pd.c[i];
    } else {
      phrase_len = (frase >= 8) ? 8 : 4;
      const Mode &md = MODES[fd.mode];
      int prev = -1;
      for (int i = 0; i < phrase_len - 1; ++i) {
        int k;
        const bool approach = (i == phrase_len - 2) || (phrase_len == 8 && i == 2);
        if (approach && rnd100(seed, i, 20) < 70) k = fd.cadential;
        else if (phrase_len == 8 && i == 3 && rnd100(seed, i, 21) < 50) k = 5;
        else k = (int)(hash(seed, i, 40) % 5);
        if (k == prev) k = (k + 1) % 5;
        prev = k;
        phrase[i] = {(int8_t)((md.tonic + fd.pool[k].rel) % 12), fd.pool[k].q, fd.mode};
      }
      phrase[phrase_len - 1] = {md.tonic, fd.pool[5].q, fd.mode};
    }
    Reharmonize(seed);
  }

  void Reharmonize(uint32_t seed) {
    if (reharm <= 0) return;
    for (int i = 0; i < phrase_len; ++i) {                      // 1: colour with sevenths
      Chord &c = phrase[i];
      const bool phryg = MODES[c.m].kind == PHRYG;
      if (c.q == MIN) c.q = MIN7;
      else if (c.q == MAJ && !phryg) c.q = MAJ7;
    }
    if (reharm >= 2) {                                          // 2: secondary dominants
      for (int i = 0; i + 1 < phrase_len; ++i) {
        const Chord &nx = phrase[i + 1];
        if (phrase[i].root == nx.root || rnd100(seed, i, 50) >= 45) continue;
        phrase[i] = {(int8_t)((nx.root + 7) % 12), DOM7, nx.m};
      }
    }
    if (reharm >= 3) {                                          // 3: tritone subs, borrowed iv
      for (int i = 0; i < phrase_len; ++i) {
        Chord &c = phrase[i];
        const int tonic = MODES[c.m].tonic;
        if (c.q == DOM7 && rnd100(seed, i, 60) < 40) c.root = (int8_t)((c.root + 6) % 12);
        else if ((c.q == MAJ || c.q == MAJ7) && ((c.root - tonic + 12) % 12) == 5 && rnd100(seed, i, 61) < 50) c.q = MIN7;
      }
    }
  }

  // ------------------------------------------------ melody
  int ScaleSize() const { return CurMode().kind == PENTA ? 5 : 7; }
  const int8_t *ScaleOf() const {
    switch (CurMode().kind) { case PHRYG: return SC_PHRYG; case MAJOR: return SC_MAJOR; case PENTA: return SC_PENTA;
                              case NMIN: return SC_NMIN; default: return SC_HMIN; }
  }
  int SemisOf(int position) const {
    const Mode &md = CurMode();
    const int n = ScaleSize();
    const int deg = ((position % n) + n) % n, oct = (position - deg) / n;
    int s = ScaleOf()[deg];
    const Chord &c = CurChord();
    if (md.kind == PHRYG && deg == 2 && (c.root % 12) == md.tonic && c.q != MIN && c.q != MIN7) s = 4;
    return md.tonic + oct * 12 + s;
  }
  bool IsChordTone(int semis) const {
    int8_t iv[4]; const int n = ChordTones(iv);
    const int pc = ((semis - CurChord().root) % 12 + 12) % 12;
    for (int i = 0; i < n; ++i) if (iv[i] == pc) return true;
    return false;
  }

  // the melody's random index: on some beats it reuses the previous beat's draws, so the figure repeats
  int MotifIdx(int idx, int per) const {
    const int b = idx / per;
    if (b > 0 && rnd100(mkey ^ 0x30F1u, b, 14) < MP().motif) return idx - per;
    return idx;
  }
  int Step(int idx) {
    const int r = rnd100(mkey ^ 0x5151u, idx, 10);
    if (perfil == 1 && !(Def().flags & F_ELEC)) {     // Techno profile on a traditional style: the riff
      if (r < 45) return 0;
      if (r < 62) return -1;
      if (r < 79) return 1;
      if (r < 87) return (pos + ScaleSize() <= range) ? ScaleSize() : -ScaleSize();
      if (r < 94) return -2;
      return 2;
    }
    const MelProfile &m = MP();
    int w[7] = {m.rep, m.dn1, m.up1, m.dn2, m.up2, m.leap, m.oct};
    const int target = range * m.center / 100;
    if (pos > target + 2) { w[2] /= 2; w[4] /= 2; w[1] += w[1] / 2; w[3] += w[3] / 2; }       // lean back to its register
    else if (pos < target - 2) { w[1] /= 2; w[3] /= 2; w[2] += w[2] / 2; w[4] += w[4] / 2; }
    int total = 0; for (int i = 0; i < 7; ++i) total += w[i];
    int x = (int)(hash(mkey ^ 0x5152u, idx, 10) % (uint32_t)(total > 0 ? total : 1)), k = 0;
    while (k < 6 && x >= w[k]) { x -= w[k]; k++; }
    const int n = ScaleSize();
    switch (k) {
      case 0: return 0;
      case 1: return -1;
      case 2: return 1;
      case 3: return -2;
      case 4: return 2;
      case 5: { const int l = (n == 5 ? 2 : 3) + (rnd100(mkey ^ 0x1EA9u, idx, 11) < 50 ? 1 : 0);   // a 4th or a 5th
                return (pos <= target) ? l : -l; }
      default: return (pos + n <= range) ? n : -n;
    }
  }
  // moves the melody one step and keeps it inside the range; strong notes land on chord tones
  void Move(int step, bool accent, bool remate, int idx) {
    if (remate_cycle && step > 0) step = -step;
    pos += step;
    if (pos < 0) pos = (step < -2) ? pos + ScaleSize() : 1;
    if (pos > range) pos = (step > 2) ? pos - ScaleSize() : range - 1;
    if (pos < 0) pos = 0;
    if (pos > range) pos = range;
    if (remate) {
      int best = pos, bestd = 99;
      for (int q = 0; q <= range; ++q) {
        if (((SemisOf(q) - CurChord().root) % 12 + 12) % 12 != 0) continue;
        const int dd = q > pos ? q - pos : pos - q;
        if (dd < bestd) { bestd = dd; best = q; }
      }
      pos = best;
    } else if ((accent || MP().chord >= 100) && rnd100(mkey ^ 0x7777u, idx, 11) < MP().chord) {
      for (int dd = 0; dd <= 3; ++dd) {
        if (pos - dd >= 0 && IsChordTone(SemisOf(pos - dd))) { pos -= dd; break; }
        if (pos + dd <= range && IsChordTone(SemisOf(pos + dd))) { pos += dd; break; }
      }
    }
  }

  // Acid: a 303 line on the semicorcheas. Notas = how many steps play, Adorno = accents, Empuje = slides
  void AcidLine(Event &ev, int s0) {
    const StyleDef &d = Def();
    int n = 0;
    for (int h = 0; h < 2; ++h) {
      const int s = s0 + h, idx = mp * 2 + h;
      const bool kick = d.a16 & (1ull << s);
      const int chance = kick ? 80 : 20 + notas * 7 / 10;
      if (rnd100(mkey ^ 0xAC1Du, MotifIdx(idx, d.beat * 2), 15) >= chance) continue;
      const int mi = MotifIdx(idx, d.beat * 2);
      Move(Step(mi), kick, false, mi);
      ev.mel_at[n] = (uint8_t)(h * 3); ev.mel_semis[n] = (int8_t)SemisOf(pos);
      ev.mel_notes |= 1u << n;
      if (rnd100(mkey ^ 0xACC0u, mi, 16) < 10 + adorno * 6 / 10) ev.mel_acc |= 1u << n;
      if (rnd100(mkey ^ 0x511Du, mi, 17) < empuje * 6 / 10) ev.mel_tie |= 1u << n;
      n++;
    }
    if (n) melody_semis = ev.mel_semis[n - 1];
    ev.mel_n = (uint8_t)n; ev.mel_main = 0;
  }

  // Semicorchea lines of the electronic styles. Each melody seed is a riff: a rhythm figure and a
  // short cell of pitches around the root that repeats every beat or two.
  //  Techno: bass lead that dodges the kick (offbeat, gallop, rolling)
  //  Minimal: few short syncopated notes, mostly the root
  //  Electro: broken 808-style bass, octave jumps
  //  Dub techno: the chord stab, its notes on the stab hits
  // Densidad picks busier figures, Notas adds or removes notes, Adorno = accents.
  struct LineDef { uint16_t fig[8]; int8_t iv[8]; uint8_t root_lean; };
  void BassLine(Event &ev, int s0) {
    static const LineDef TECHNO = {{0x4444, 0xAAAA, 0x6666, 0xCCCC, 0x4E4E, 0xAEAE, 0xE6E6, 0xEEEE}, {0, 0, 0, 12, 0, 7, 10, 3}, 80};
    static const LineDef MINIMAL = {{0x0404, 0x0808, 0x4040, 0x4444, 0x0848, 0x4848, 0x8484, 0x4C4C}, {0, 0, 0, 0, 7, 12, 0, 10}, 90};
    static const LineDef ELECTRO = {{0x0481, 0x0909, 0x1449, 0x4949, 0x2449, 0x4C4D, 0x5249, 0x6D49}, {0, 0, 12, 12, 0, 7, 10, 3}, 60};
    const int st = active_style;
    if (st == S_DUB) { DubBass(ev, s0); return; }
    const LineDef &L = st == S_MINIMAL ? MINIMAL : st == S_ELECTRO ? ELECTRO : TECHNO;
    int fig_i = (density * (st == S_MINIMAL ? 8 : 6) / 100) + (int)(hash(bkey, 0x7EC, 1) % 3) - 1;
    fig_i = fig_i < 0 ? 0 : fig_i > 7 ? 7 : fig_i;
    const uint16_t fig = L.fig[fig_i];
    const int cell = (hash(bkey, 0x7EC, 2) & 1) ? 4 : 8;           // the pitch cell repeats every beat or every 2 beats
    const int base = ((RootSemis() % 12) + 12) % 12;
    int n = 0;
    for (int h = 0; h < 2; ++h) {
      const int s = s0 + h;
      const bool in_fig = fig & (1u << (s & 15));
      int chance = in_fig ? 70 + notas * 30 / 100 : (notas > 60 ? (notas - 60) * 3 / 4 : 0);
      if (st == S_MINIMAL && !in_fig) chance /= 3;
      if (rnd100(bkey ^ 0x7EC0u, s, 3) >= chance) continue;
      const int k = s % cell;
      int iv = L.iv[hash(bkey ^ 0x7EC1u, k, 4) % 8];
      if (k == 0 || (s & 3) == 2) iv = (rnd100(bkey ^ 0x7EC2u, k, 5) < L.root_lean) ? 0 : iv;   // the cell start and the offbeat lean on the root
      ev.bass_at[n] = (uint8_t)(h * 3); ev.bass_semis[n] = (int8_t)(base + iv);
      if ((s & 3) == 2 ? rnd100(bkey ^ 0x7EC3u, s, 6) < 30 + adorno / 2 : rnd100(bkey ^ 0x7EC4u, s, 7) < adorno / 5) ev.bass_acc |= 1u << n;
      n++;
    }
    ev.bass_n = (uint8_t)n;
  }
  // Dub techno: the melody plays the chord stab, a chord tone on each stab hit (with the 9th now and then)
  // Dub techno bass: a deep root on the 1 and some syncopated pushes, root / fifth / octave
  void DubBass(Event &ev, int s0) {
    static const uint16_t FIG[4] = {0x0101, 0x0901, 0x0111, 0x4101};
    const uint16_t fig = FIG[hash(bkey, 0xDB, 1) % 4];
    const int base = ((RootSemis() % 12) + 12) % 12;
    int n = 0;
    for (int h = 0; h < 2; ++h) {
      const int s = s0 + h;
      if (!(fig & (1u << (s & 15))) || rnd100(bkey ^ 0xDB0u, s, 2) >= 80 + notas / 5) continue;
      const int r = rnd100(bkey ^ 0xDB1u, s & 7, 3);
      ev.bass_at[n] = (uint8_t)(h * 3); ev.bass_semis[n] = (int8_t)(base + ((s & 15) == 0 || r < 70 ? 0 : r < 85 ? 7 : 12));
      n++;
    }
    ev.bass_n = (uint8_t)n;
  }
  // the lead of the electronic styles, an octave over the bass. Its own rhythm figure and a short
  // cell of notes (chord tones and minor pentatonic) that cycles over the figure, so it turns around
  // the bar like a hypnotic synth riff.
  //  Techno: syncopated 3+3+2 riffs.  Minimal: a few blips.  Electro: arpeggios.  Dub: the chord stab.
  void LeadLine(Event &ev, int s0) {
    const int st = active_style;
    if (st == S_DUB) { DubLine(ev, s0); return; }
    static const uint16_t TECHNO[8] = {0x0410, 0x2222, 0x2492, 0x9249, 0x0924, 0x4924, 0xA4A4, 0xB6DB};
    static const uint16_t MINIMAL[8] = {0x0010, 0x0400, 0x0410, 0x1008, 0x0084, 0x2010, 0x0441, 0x4210};
    static const uint16_t ELECTRO[8] = {0x1111, 0x5555, 0x5A5A, 0x6DB6, 0x7777, 0xAAAA, 0xEEEE, 0xFFFF};
    const uint16_t *F = st == S_MINIMAL ? MINIMAL : st == S_ELECTRO ? ELECTRO : TECHNO;
    int fi = density * 7 / 100 + (int)(hash(mkey, 0x1EAD, 1) % 3) - 1;
    fi = fi < 0 ? 0 : fi > 7 ? 7 : fi;
    const uint16_t fig = F[fi];
    // the cell: 3 to 5 notes
    int8_t iv[4]; const int nt = ChordTones(iv);
    static const int8_t PENT[5] = {0, 3, 5, 7, 10};
    const int cell_len = 3 + (int)(hash(mkey, 0x1EAD, 2) % 3);
    const int base = ((RootSemis() % 12) + 12) % 12 + 12;
    int n = 0;
    for (int h = 0; h < 2; ++h) {
      const int s = s0 + h, pos = s & 15;
      if (!(fig & (1u << pos))) continue;
      if (rnd100(mkey ^ 0x1EA0u, pos, 3) >= 55 + notas * 45 / 100) continue;
      // which note of the cell: count the figure's hits before this one, so the cell cycles over the figure
      int k = 0; for (int j = 0; j < pos; ++j) k += (fig >> j) & 1;
      int semis;
      if (st == S_ELECTRO) {                                   // arpeggio up the chord, over 2 octaves
        const int m = k % (nt * 2); semis = base + iv[m % nt] + 12 * (m / nt);
      } else {
        const uint32_t r = hash(mkey ^ 0x1EA1u, k % cell_len, 4);
        semis = base + ((r & 3) ? iv[(r >> 2) % nt] : PENT[(r >> 4) % 5]) + ((r >> 8) % 6 == 0 ? 12 : 0);
      }
      ev.mel_at[n] = (uint8_t)(h * 3); ev.mel_semis[n] = (int8_t)semis; ev.mel_notes |= 1u << n;
      if (rnd100(mkey ^ 0x1EA2u, pos, 5) < adorno / 2) ev.mel_acc |= 1u << n;
      n++;
    }
    if (n) melody_semis = ev.mel_semis[n - 1];
    ev.mel_n = (uint8_t)n; ev.mel_main = 0;
  }
  void DubLine(Event &ev, int s0) {
    const StyleDef &d = Def();
    int8_t iv[4]; const int nt = ChordTones(iv);
    const int base = ((RootSemis() % 12) + 12) % 12 + 12;
    int n = 0;
    for (int h = 0; h < 2; ++h) {
      const int s = s0 + h;
      const bool stab = d.pref16 & (1ull << (s & 15));
      const int chance = stab ? 60 + notas * 40 / 100 : (notas > 70 ? (notas - 70) / 2 : 0);
      if (rnd100(mkey ^ 0xD0B0u, s, 3) >= chance) continue;
      const int r = (int)(hash(mkey ^ 0xD0B1u, s % 8, 4) % 10);
      const int semis = r < 8 ? base + iv[r % (nt > 0 ? nt : 1)] : base + 14;     // a chord tone, or the 9th
      ev.mel_at[n] = (uint8_t)(h * 3); ev.mel_semis[n] = (int8_t)semis; ev.mel_notes |= 1u << n;
      if (stab && rnd100(mkey ^ 0xD0B2u, s, 6) < adorno) ev.mel_acc |= 1u << n;
      n++;
    }
    if (n) melody_semis = ev.mel_semis[n - 1];
    ev.mel_n = (uint8_t)n; ev.mel_main = 0;
  }

  void Melody(Event &ev, bool accent) {
    const int mi = MotifIdx(mp, Def().beat);
    Move(Step(mi), accent, ev.remate, mi);
    melody_semis = SemisOf(pos);

    int n = 0;
    auto add = [&](int at, int semis) { ev.mel_at[n] = (uint8_t)at; ev.mel_semis[n] = (int8_t)semis; n++; };

    if (mel_style == 0) {
      // Clasica (the 4.1 melody): only a short roce from the note above, only on accents,
      // resolving at the first third of the corchea so the note stays on the beat
      if (accent && rnd100(mkey ^ 0x6ACEu, mp, 9) < adorno / 2 && pos + 1 <= range + 2) {
        add(0, SemisOf(pos + 1));
        add(2, melody_semis);
      } else {
        add(0, melody_semis);
      }
      ev.mel_n = (uint8_t)n; ev.mel_main = (uint8_t)(n - 1); ev.mel_notes = 1u << ev.mel_main;
      return;
    }

    // Ornamentada (4.4 ornaments, fixed in 4.5): the main note always lands on the beat and
    // the ornament is a short turn after it, back on the note before the corchea ends
    add(0, melody_semis);
    const int orn_chance = adorno * (accent ? 70 : 35) / 100;
    if (rnd100(mkey ^ 0x0A0Au, mp, 12) < orn_chance) {
      const int kind = rnd100(mkey ^ 0x0B0Bu, mp, 13);
      switch (Def().orn) {
        case H_FLAM:
          if (ev.remate || kind < 35) { add(3, SemisOf(pos + 2)); add(4, SemisOf(pos + 1)); add(5, melody_semis); } // melisma
          else if (kind < 70) { add(3, SemisOf(pos + 1)); add(4, melody_semis); }                              // mordente
          else { add(4, SemisOf(pos + 1)); add(5, melody_semis); }                                           // roce
          break;
        case H_ANDI:
          if (kind < 60) { add(4, SemisOf(pos - 1)); add(5, melody_semis); }                                  // flam from below
          else { add(3, SemisOf(pos + 1)); add(4, melody_semis); }                                           // floreo
          break;
        case H_PASI:
        case H_BOLE:
          if (kind < 50) { add(4, melody_semis + 1); add(5, melody_semis); }                                  // chromatic appoggiatura
          else { add(3, SemisOf(pos - 1)); add(4, SemisOf(pos + 1)); add(5, melody_semis); }                // turn
          break;
        default:
          add(4, melody_semis - 1); add(5, melody_semis);                                                   // chromatic approach
          break;
      }
    }
    ev.mel_n = (uint8_t)n; ev.mel_main = 0; ev.mel_notes = 1u;
  }
};

} // namespace compas
