/* Solution C-13-EX1: Placement new Packet{0x1234, 0x5678} at $2000 + Seg7 '0' ($3F) */
#include "../../runtime/easy68k.h"

inline void* operator new(size_t, void* p) noexcept { return p; }

struct Packet {
  uint16_t a;
  uint16_t b;
  Packet(uint16_t x, uint16_t y) : a(x), b(y) {}
};

int main() {
  Packet* pkt = new ((void*)0x2000UL) Packet(0x1234, 0x5678);
  EASY68K_SEG7_ADDR[0] = 0x3F;
  return (int)((uint32_t)pkt->a + (uint32_t)pkt->b);
}
