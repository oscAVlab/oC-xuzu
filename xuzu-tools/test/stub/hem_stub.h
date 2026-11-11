// minimal HemisphereApplet stand-in to run the xuzu applets on the computer
#pragma once
#include "Arduino.h"
#include <string>
#include <vector>
namespace OC { namespace CORE { inline uint32_t ticks = 0; } }
#define HEMISPHERE_3V_CV 4608
#define HEMISPHERE_MAX_INPUT_CV 9216
#define LEFT_HEMISPHERE 0
#define RIGHT_HEMISPHERE 1
enum { HELP_DIGITAL1, HELP_DIGITAL2, HELP_CV1, HELP_CV2, HELP_OUT1, HELP_OUT2, HELP_EXTRA1, HELP_EXTRA2, HELP_LABEL_COUNT };
struct PackLocation { size_t location; size_t size; };
constexpr void Pack(uint64_t &data, const PackLocation p, const uint64_t value) { data |= (value & ((1ull << p.size) - 1)) << p.location; }
constexpr int Unpack(const uint64_t &data, const PackLocation p) { return (data >> p.location) & ((1ull << p.size) - 1); }
inline std::vector<std::string> screen;
class HemisphereApplet {
public:
  int hemisphere = 0; bool editing = false; int in[2] = {0, 0}; bool tr[2] = {false, false}; int out[2] = {0, 0}; int trigs = 0;
  const char *help[HELP_LABEL_COUNT] = {};
  virtual ~HemisphereApplet() {}
  bool Clock(int ch) { return tr[ch]; }
  int In(int ch) { return in[ch]; }
  int DetentedIn(int ch) { return in[ch]; }
  void Out(int ch, int v) { out[ch] = v; }
  void ClockOut(int ch, int = 100) { trigs++; out[ch] = 1; }
  void GateOut(int ch, bool h) { out[ch] = h; }
  bool EditMode() { return editing; }
  template <class T> void MoveCursor(T &c, int d, int max) { c = constrain(c + d, 0, max); }
  void gfxPrint(int, int, const char *s) { if (strlen(s) > 10) { printf("TEXTO LARGO: %s\n", s); abort(); } screen.push_back(s); }
  void gfxPrint(int x, int y, int n) { char b[12]; snprintf(b, 12, "%d", n); gfxPrint(x, y, b); }
  void gfxRect(int, int, int, int) {} void gfxPixel(int, int) {} void gfxLine(int, int, int, int) {}
  void gfxDottedLine(int, int, int, int) {} void gfxInvert(int, int, int, int) {} void gfxCursor(int, int, int) {}
};
