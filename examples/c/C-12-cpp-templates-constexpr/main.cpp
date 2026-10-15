/*
 * C-12: Templates and constexpr — Zero-Cost Abstractions on the 68000
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

constexpr uint32_t factorial_ct(uint32_t n) {
  uint32_t acc = 1;
  for (uint32_t i = 2; i <= n; ++i) {
    acc *= i;
  }
  return acc;
}

template <int Shift>
constexpr int32_t scale_pow2(int32_t x) {
  return x << Shift;
}

int main() {
  constexpr uint32_t kFact6 = factorial_ct(6); /* 720 at compile time */
  int32_t runtime_x = 5;
  return (int)(kFact6 + (uint32_t)scale_pow2<3>(runtime_x)); /* 720 + 40 = 760 */
}
