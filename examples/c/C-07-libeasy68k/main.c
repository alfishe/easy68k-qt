/*
 * C-07: Writing libeasy68k with GCC Extended Inline Assembly
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

int main(void) {
  uint32_t v = 255;
  easy68k_puts("val=");
  easy68k_print_int((int32_t)v);
  easy68k_puts(" hex=");
  easy68k_print_radix(v, 16);
  easy68k_println("");
  return (int)v;
}
