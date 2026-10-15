/* Exercise C-06-EX1: Sum s->x + s->y from struct Sprite at $2000 */
#include "../../runtime/easy68k.h"

struct Sprite {
  uint8_t  id;
  int16_t  x;
  int16_t  y;
  uint32_t color;
};

int main(void) {
  const struct Sprite* s = (const struct Sprite*)0x2000UL;
  /* TODO: return s->x + s->y (120 + 80 = 200) */
  (void)s;
  return 0;
}
