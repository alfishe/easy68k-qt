/*
 * C-06: Struct Layout, Alignment Padding, and OFFSET Views
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

struct Sprite {
  uint8_t  id;     /* offset 0 (+ 1 byte padding at offset 1) */
  int16_t  x;      /* offset 2 */
  int16_t  y;      /* offset 4 */
  uint32_t color;  /* offset 6; total sizeof = 10 bytes */
};

int main(void) {
  volatile struct Sprite* s = (volatile struct Sprite*)0x2000UL;
  s->id    = 7;
  s->x     = 120;
  s->y     = 80;
  s->color = 0x00FF8000UL;
  return (int)sizeof(struct Sprite); /* 10 */
}
