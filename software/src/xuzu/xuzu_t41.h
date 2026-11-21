// o_C T4.1 (Tunefish / O.R.N.8, "Phažerville"): what changes in the xuzu apps on that hardware.
// Controls: left encoder turn = parameter, press = next page.  Right encoder turn = edit, press = quick action.
//           A / B = page back / forward.  X / Y = per app (track, slot, remate / fill).  Center: next preset, hold = save.
// Outputs E-H and CV inputs 5-8 are used by the apps; the T3.2 build keeps its 4 + 4 and the left-encoder layout.
#pragma once
#if defined(ARDUINO_TEENSY41)
#define XUZU_T41(...) __VA_ARGS__
#define XUZU_T41_ELSE(a, b) a
#define XUZU_IS_T41 1
#else
#define XUZU_T41(...)
#define XUZU_T41_ELSE(a, b) b
#define XUZU_IS_T41 0
#endif
namespace xuzu {
// next slot after `from` that holds a preset (wraps), -1 if none
template <class F> inline int NextPreset(int from, int n, F valid) {
  for (int k = 1; k <= n; ++k) { const int s = (from + k) % n; if (valid(s)) return s; }
  return -1;
}
// CV input of the i-th assignable CV: 0..2 = CV1..CV3, 3..6 = CV5..CV8 (CV4 stays preset select)
inline int CvInput(int i) { return i < 3 ? i : i + 1; }
}
