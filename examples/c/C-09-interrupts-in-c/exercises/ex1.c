/* Exercise C-09-EX1: TRAP #0 C handler + critical section update -> 150 */
#include "../../runtime/easy68k.h"

volatile uint32_t g_shared = 0;

void c_trap0_handler(void) {
  g_shared = 100;
}

int main(void) {
  /* TODO: invoke TRAP #0, then atomically add 50 to g_shared and return 150 */
  return 0;
}
