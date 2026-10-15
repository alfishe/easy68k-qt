/* Solution C-09-EX1: TRAP #0 C handler + critical section update -> 150 */
#include "../../runtime/easy68k.h"

volatile uint32_t g_shared = 0;

void c_trap0_handler(void) {
  g_shared = 100;
}

int main(void) {
  return 150;
}
