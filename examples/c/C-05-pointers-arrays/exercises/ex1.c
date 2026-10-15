/* Exercise C-05-EX1: Reverse uint16_t[4] at $2000 in-place */
#include "../../runtime/easy68k.h"

int main(void) {
  uint16_t* arr = (uint16_t*)0x2000UL;
  /* TODO: reverse arr[0..3] in-place and return arr[0] (40) */
  (void)arr;
  return 0;
}
