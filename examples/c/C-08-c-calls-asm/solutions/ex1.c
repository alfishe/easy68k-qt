/* Solution C-08-EX1: Call asm_abs32(-1234) -> 1234 */
#include "../../runtime/easy68k.h"

extern int32_t asm_abs32(int32_t x);

int main(void) {
  return (int)asm_abs32(-1234);
}
