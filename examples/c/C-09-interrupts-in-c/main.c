/*
 * C-09: Interrupts in C — ISR Trampoline, volatile, and Critical Sections
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

volatile uint32_t g_ticks = 0;

/* Called from irq2_trampoline (which saves D0-D1/A0-A1 and returns via RTE). */
void c_timer_isr(void) {
  g_ticks = g_ticks + 1;
}

int main(void) {
  /* Enable exceptions, install irq2_trampoline at $0068, wait for 1 tick */
  return (int)g_ticks;
}
