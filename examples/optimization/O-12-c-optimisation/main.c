/*
 * O-12: Writing 68000-Friendly C (int16_t + countdown DBRA + MULS.W)
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../c/runtime/easy68k.h"

int32_t scale_sum_68k_friendly(const int16_t* __restrict src, int16_t count, int16_t factor) {
  int32_t sum = 0;
  for (int16_t n = count; --n >= 0; ) { /* lowers directly to DBRA! */
    sum += (int32_t)(*src++) * (int32_t)factor; /* lowers to MULS.W (A0)+,Dn */
  }
  return sum;
}
