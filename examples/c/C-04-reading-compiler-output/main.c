/*
 * C-04: Reading Compiler Output (-O0 vs -O2 sum of squares)
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

int32_t sum_sq_O2(int16_t n) {
  int32_t sum = 0;
  for (int16_t i = 1; i <= n; ++i) {
    sum += (int32_t)i * (int32_t)i;
  }
  return sum;
}

int main(void) {
  return (int)sum_sq_O2(6); /* 1 + 4 + 9 + 16 + 25 + 36 = 91 */
}
