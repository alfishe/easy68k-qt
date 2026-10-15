/*
 * O-11: Compiler vs Hand Assembly — Array Sum + Max
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../c/runtime/easy68k.h"

void sum_and_max(const int16_t* data, int16_t count, int32_t* out_sum, int16_t* out_max) {
  int32_t sum = 0;
  int16_t mx = data[0];
  for (int16_t i = 0; i < count; ++i) {
    int16_t v = data[i];
    sum += v;
    if (v > mx) mx = v;
  }
  *out_sum = sum;
  *out_max = mx;
}
