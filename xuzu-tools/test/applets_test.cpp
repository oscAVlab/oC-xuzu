#include "hem_stub.h"
#include "xuzu/xuzu_flash.h"
namespace xuzu { bool EuclidLoadPreset(int) { return false; } bool BrainsLoadPreset(int) { return false; } }
#include "xuzu/HEM_Compas.h"
#include "xuzu/HEM_EuclX4.h"
#include "xuzu/HEM_Brains.h"
euclid4::Engine euclid_engine;
brains::Engine brains_engine;
static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FALLA: " __VA_ARGS__); printf("\n"); } } while (0)
struct TC : CompasApplet { using CompasApplet::cursor; using CompasApplet::Change; using CompasApplet::NumParams; using CompasApplet::SetHelp;
  void Draw() { View(); } };
struct TE : EuclX4 { using EuclX4::cursor; using EuclX4::Change; using EuclX4::NumParams; void Draw() { View(); } };
struct TB : Brains { using Brains::cursor; using Brains::Change; using Brains::NumParams; void Draw() { View(); } };

template <class A> void Sweep(A &a, const char *name) {   // every parameter through its whole range, drawing each time
  for (int p = 0; p < a.NumParams(); ++p) {
    a.cursor = p;
    for (int d = 0; d < 40; ++d) { a.Change(p, 1); a.editing = d & 1; a.Draw(); }
    for (int d = 0; d < 80; ++d) { a.Change(p, -1); a.Draw(); }
    for (int d = 0; d < 3; ++d) a.Change(p, 1);
  }
  printf("%s: %d parametros recorridos\n", name, a.NumParams());
}
int main() {
  brains_engine.Init();
  TC c; c.Start(); Sweep(c, "Compas");
  TE e; e.Start(); Sweep(e, "Eucl x4");
  TB b; b.Start(); Sweep(b, "Brains");
  // Compas: the preset brings back the same settings and the same music
  {
    for (int p = 0; p < c.NumParams(); ++p) for (int i = 0; i < p % 4; ++i) c.Change(p, 1);
    const uint64_t d = c.OnDataRequest();
    TC c2; c2.Start(); c2.OnDataReceive(d);
    CHECK(c2.OnDataRequest() == d, "Compas: el preset no vuelve igual (%llx / %llx)", (unsigned long long)d, (unsigned long long)c2.OnDataRequest());
    c.OnDataReceive(d);
    std::string s1, s2;
    for (int t = 0; t < 2000 * 64; ++t) {
      OC::CORE::ticks++;
      c.tr[0] = c2.tr[0] = (t % 2000) == 0;
      c.trigs = c2.trigs = 0; c.Controller(); c2.Controller();
      s1 += (char)('0' + c.trigs) ; s1 += std::to_string(c.out[1]); s2 += (char)('0' + c2.trigs); s2 += std::to_string(c2.out[1]);
    }
    CHECK(s1 == s2, "Compas: dos applets con el mismo preset no suenan igual");
    c.SetHelp();
  }
  // Compas: the style survives the 5-bit packing (styles above 15)
  { TC c3; c3.Start(); for (int i = 0; i < 17; ++i) c3.Change(TC::P_STYLE, 1); TC c4; c4.OnDataReceive(c3.OnDataRequest());
    CHECK(c4.OnDataRequest() == c3.OnDataRequest(), "Compas: estilo 18 no se guarda"); }
  // Eucl x4 and Brains: preset roundtrip
  { const uint64_t d = e.OnDataRequest(); TE e2; e2.OnDataReceive(d); CHECK(e2.OnDataRequest() == d, "Eucl x4: el preset no vuelve igual"); }
  { const uint64_t d = b.OnDataRequest(); TB b2; b2.OnDataReceive(d); CHECK(b2.OnDataRequest() == d, "Brains: el preset no vuelve igual"); }
  // Eucl x4 with CV1 on hits of all tracks: modulation reaches the engine and goes away without CV
  {
    TE x; x.Start(); x.cursor = 0;
    x.Change(TE::P_CV1, 1); x.Change(TE::P_CV1T, 5);    // Golpes, Todas
    x.in[0] = 7680;
    OC::CORE::ticks += 10; x.Controller();
    CHECK(euclid_engine.hm_hits[0] == 16 && euclid_engine.hm_hits[3] == 16, "Eucl x4: CV1 a golpes no llega (%d)", euclid_engine.hm_hits[0]);
    x.in[0] = 0; OC::CORE::ticks++; x.Controller();
    CHECK(euclid_engine.hm_hits[0] == 0, "Eucl x4: la modulacion no se borra");
  }
  printf(fails ? "%d FALLAS\n" : "todo bien\n", fails);
  return fails != 0;
}
