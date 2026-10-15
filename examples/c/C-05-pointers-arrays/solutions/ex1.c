/* Solution C-05-EX1: Reverse uint16_t[4] at $2000 in-place */
#include "../../runtime/easy68k.h"

int main(void) {
  uint16_t* lo = (uint16_t*)0x2000UL;
  uint16_t* hi = lo + 3;
  while (lo < hi) {
    uint16_t tmp = *lo;
    *lo++ = *hi;
    *hi-- = tmp;
  }
  return (int)((uint16_t*)0x2000UL)[0];
}
