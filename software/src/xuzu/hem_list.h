// Parameter list for the xuzu Hemisphere applets: every setting of the applet in half a screen.
// Top row: the full value of the selected parameter. Below: 4 rows "name   value" that scroll.
// Turn the encoder to move, press to edit (the value gets underlined), press again to leave.
#pragma once
#include <string.h>
#include <stdint.h>

namespace xuzu {
// who drives the shared engines in Hemispheres this tick (so two applets never clock them twice)
inline uint32_t eucl_seen_tick = 0, brains_seen_tick = 0;
inline bool Recent(uint32_t t, uint32_t now) { return now - t < 3; }
// preset loaders of the apps, defined in APP_EUCLID4.h / APP_BRAINSSEQ.h
bool EuclidLoadPreset(int slot);
bool BrainsLoadPreset(int slot);
// small int -> text, no printf
inline void Itoa(int v, char *b) {
  char t[8]; int n = 0; bool neg = v < 0; unsigned u = neg ? -v : v;
  do { t[n++] = '0' + u % 10; u /= 10; } while (u && n < 7);
  int i = 0; if (neg) b[i++] = '-';
  while (n) b[i++] = t[--n];
  b[i] = 0;
}
inline void Copy(char *b, const char *s, int max) { int i = 0; for (; s[i] && i < max; ++i) b[i] = s[i]; b[i] = 0; }
inline void Signed(int v, char *b) { if (v > 0) { b[0] = '+'; Itoa(v, b + 1); } else Itoa(v, b); }
inline int Bi(int cv, int range) { return cv * range / 7680; }          // +-5 V -> +-range
inline int Uni(int cv, int n) { return cv <= 0 ? 0 : (cv * n / 7681 < n ? cv * n / 7681 : n - 1); }   // 0..5 V -> 0..n-1
}

class XuzuListApplet : public HemisphereApplet {
protected:
  int cursor = 0;
  virtual int NumParams() = 0;
  virtual const char *Name(int i) = 0;                  // up to 5 letters
  virtual void Value(int i, char *b, bool full) = 0;    // short: up to 5 letters, full: up to 9
  virtual void Change(int i, int dir) = 0;
  virtual bool Top() { return false; }                  // draws its own top row while not editing

  void DrawList(bool activity) {
    char b[16];
    if (EditMode() || !Top()) { Value(cursor, b, true); b[9] = 0; gfxPrint(1, 15, b); }
    if (activity) gfxRect(59, 15, 4, 7);
    gfxDottedLine(0, 24, 63, 24);
    const int n = NumParams();
    int first = cursor - 1; if (first > n - 4) first = n - 4; if (first < 0) first = 0;
    for (int r = 0; r < 4 && first + r < n; ++r) {
      const int i = first + r, y = 27 + r * 9;
      gfxPrint(1, y, Name(i));
      Value(i, b, false); b[5] = 0;
      const int w = 6 * (int)strlen(b), x = 64 - w;
      gfxPrint(x, y, b);
      if (i == cursor) { if (EditMode()) gfxCursor(x, y + 8, w); else gfxInvert(0, y - 1, 31, 9); }
    }
  }
  void ListEncoder(int dir) {
    if (!EditMode()) { MoveCursor(cursor, dir, NumParams() - 1); return; }
    Change(cursor, dir);
  }
};
