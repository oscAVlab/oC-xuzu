#pragma once
namespace xuzu {
static constexpr int NUM_PRESETS = 16;
inline int CVPreset(int cv, int five_volts, int &last) {
  const int floor = five_volts / 20;
  if (cv < floor) { last = -1; return -1; }
  int v = (cv - floor) * NUM_PRESETS / (five_volts - floor + 1);
  if (v >= NUM_PRESETS) v = NUM_PRESETS - 1;
  // hysteresis: only move when clearly inside the next zone
  if (last >= 0 && v != last) {
    const int zone = (five_volts - floor) / NUM_PRESETS;
    const int center_last = floor + last * zone + zone / 2;
    const int d = cv > center_last ? cv - center_last : center_last - cv;
    if (d < zone * 6 / 10) v = last;
  }
  if (v == last) return -1;
  last = v;
  return v;
}
}
