#include <cstdio>
#include <cassert>
#include "xuzu/jf_ii.h"
int main(){
  jf::Note(0, jf::Semis(12), jf::Level(50));   // 1 V, 5 V
  jf::Vox(1, 3, 4, 3);
  jf::Tick(0, 16);
  jf::Tune(0, 2, 3, 2);
  for (int i = jf::tail; i != jf::head; i = (i+1)&63) { auto &m = jf::q[i]; printf("%02x:", m.addr); for (int k=0;k<m.n;k++) printf(" %02x", m.b[k]); printf("\n"); }
  assert(jf::Semis(12)==1638); assert(jf::FromHS(1536)==1638); assert(jf::Level(100)==16383);
  for (int i=0;i<100;i++) jf::Tick(0,1);  // overflow is dropped, no crash
  printf("q=%d\n", (jf::head - jf::tail) & 63);
}
