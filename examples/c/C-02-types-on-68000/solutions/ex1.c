/* Solution C-02-EX1: Sum high and low 16-bit halves of $ABCDEF01 */
#include "../../runtime/easy68k.h"

int main(void) {
  const uint16_t* words = (const uint16_t*)0x2000UL;
  return (int)((uint32_t)words[0] + (uint32_t)words[1]);
}
