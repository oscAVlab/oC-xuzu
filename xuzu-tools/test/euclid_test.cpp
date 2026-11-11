#include "xuzu/euclid4_engine.h"
#include <cstdio>
int main() { // print every pattern as a line "p s bits"
  for (int s = 1; s <= 64; ++s) for (int p = 0; p <= s; ++p) {
    uint8_t o[64]; memset(o, 9, 64); euclid4::Euclid(p, s, o);
    printf("%d %d ", p, s); for (int i = 0; i < s; ++i) putchar('0' + o[i]); putchar('\n');
  }
}
