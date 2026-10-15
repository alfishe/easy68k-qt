/* Solution C-01-EX1: Return g_start (100) + g_zero (0) + 23 in D0 */
#include "../../runtime/easy68k.h"

int32_t g_start = 100;
int32_t g_zero;

int main(void) {
  return (int)(g_start + g_zero + 23);
}
