/*
 * C-02: C Types on the 68000 — sizeof and Big-Endian Layout
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

int main(void) {
  volatile uint32_t* out = (volatile uint32_t*)0x2000UL;
  uint32_t sizes = ((uint32_t)sizeof(char) << 24) |
                   ((uint32_t)sizeof(short) << 16) |
                   ((uint32_t)sizeof(int) << 8) |
                   ((uint32_t)sizeof(void*));
  out[0] = sizes;        /* 0x01020404 at $2000 */
  out[1] = 0x12345678UL; /* big-endian bytes 12 34 56 78 at $2004 */
  return (int)sizes;
}
