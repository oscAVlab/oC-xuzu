#pragma once
#include <stdint.h>
#include <string.h>
struct FB {
  uint8_t px[64][128];
  void clear() { memset(px, 0, sizeof px); }
  void setPixel(int x, int y) { if (x >= 0 && x < 128 && y >= 0 && y < 64) px[y][x] = 1; }
  void drawRect(int x, int y, int w, int h) { for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) setPixel(x + i, y + j); }
  void drawFrame(int x, int y, int w, int h) { for (int i = 0; i < w; ++i) { setPixel(x + i, y); setPixel(x + i, y + h - 1); } for (int j = 0; j < h; ++j) { setPixel(x, y + j); setPixel(x + w - 1, y + j); } }
  void drawHLine(int x, int y, int w) { for (int i = 0; i < w; ++i) setPixel(x + i, y); }
  void drawCircle(int cx, int cy, int r) { int f = 1 - r, dx = 1, dy = -2 * r, x = 0, y = r;
    setPixel(cx, cy + r); setPixel(cx, cy - r); setPixel(cx + r, cy); setPixel(cx - r, cy);
    while (x < y) { if (f >= 0) { y--; dy += 2; f += dy; } x++; dx += 2; f += dx;
      setPixel(cx + x, cy + y); setPixel(cx - x, cy + y); setPixel(cx + x, cy - y); setPixel(cx - x, cy - y);
      setPixel(cx + y, cy + x); setPixel(cx - y, cy + x); setPixel(cx + y, cy - x); setPixel(cx - y, cy - x); } }
};
inline FB graphics;
