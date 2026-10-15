/*
 * C-08: Calling Hand-Written 68000 Assembly from C (and Back)
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

extern uint32_t asm_popcount32(uint32_t x);
extern int32_t  asm_call_c_double(int32_t x);

/* Called back from assembly! */
int32_t c_double(int32_t x) {
  return x * 2;
}

int main(void) {
  uint32_t bits = asm_popcount32(0xF0F055AAUL); /* 16 */
  int32_t  doubled = asm_call_c_double(21);     /* 42 */
  return (int)(bits + (uint32_t)doubled);       /* 58 */
}
