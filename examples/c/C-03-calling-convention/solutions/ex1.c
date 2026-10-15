/* Solution C-03-EX1: clamp_val(150, 0, 100) -> 100 */
#include "../../runtime/easy68k.h"

int32_t clamp_val(int32_t x, int32_t lo, int32_t hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

int main(void) {
  return (int)clamp_val(150, 0, 100);
}
