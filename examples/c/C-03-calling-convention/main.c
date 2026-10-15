/*
 * C-03: m68k C Calling Convention
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

int32_t weighted_sum(int32_t a, int32_t b, int32_t c) {
  return a * 2 + b * 3 + c * 4;
}

int main(void) {
  return (int)weighted_sum(10, 20, 30); /* 20 + 60 + 120 = 200 */
}
