/*
 * C-01: Freestanding C Toolchain — crt0, .data, .bss, and main()
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

int32_t g_base = 40;   /* .data section: initialised to 40 */
int32_t g_uninit;      /* .bss section: zeroed by crt0 before main() */

int main(void) {
  int32_t ans = g_base + g_uninit + 2;
  easy68k_println("C-01: 42");
  return (int)ans;     /* returned in D0 */
}
