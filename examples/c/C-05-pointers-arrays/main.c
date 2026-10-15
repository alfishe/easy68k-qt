/*
 * C-05: Pointers, Arrays, and volatile Memory-Mapped I/O
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

static const int32_t kValues[4] = {15, 30, 45, 60};

int main(void) {
  const int32_t* p = kValues;
  int32_t sum = 0;
  for (int i = 0; i < 4; ++i) {
    sum += *p++; /* compiles to ADD.L (A0)+,D0 */
  }
  *EASY68K_LED_ADDR = (uint8_t)sum; /* volatile write to $E00010 (150 = $96) */
  return (int)sum;
}
