/*
 * libeasy68k — Freestanding C11 / C++17 runtime for EASy68K Trap #15 I/O.
 * SPDX-License-Identifier: CC0-1.0
 */
#ifndef EASY68K_RUNTIME_H_
#define EASY68K_RUNTIME_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory-mapped hardware window default addresses (Trap #15 task 32). */
#define EASY68K_SEG7_ADDR   ((volatile uint8_t*)0x00E00000UL)
#define EASY68K_LED_ADDR    ((volatile uint8_t*)0x00E00010UL)
#define EASY68K_SWITCH_ADDR ((volatile const uint8_t*)0x00E00012UL)
#define EASY68K_BUTTON_ADDR ((volatile const uint8_t*)0x00E00014UL)

/* Program termination (Trap #15 task 9 / SIMHALT). */
static inline void easy68k_halt(void) {
#if defined(__mc68000__) || defined(__m68k__)
  __asm__ volatile(
      "moveq #9,%%d0\n\t"
      "trap #15\n\t"
      ".word 0xFFFF"
      :
      :
      : "d0", "cc", "memory");
#endif
}

/* Text I/O (Trap #15 tasks 3, 4, 5, 6, 11, 13, 14, 15). */
static inline void easy68k_println(const char* s) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 13;
  register const char* a1 __asm__("a1") = s;
  __asm__ volatile("trap #15" : "+d"(d0) : "a"(a1) : "cc", "memory");
#else
  (void)s;
#endif
}

static inline void easy68k_puts(const char* s) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 14;
  register const char* a1 __asm__("a1") = s;
  __asm__ volatile("trap #15" : "+d"(d0) : "a"(a1) : "cc", "memory");
#else
  (void)s;
#endif
}

static inline void easy68k_putchar(char c) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 6;
  register uint32_t d1 __asm__("d1") = (uint8_t)c;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
#else
  (void)c;
#endif
}

static inline void easy68k_print_int(int32_t value) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 3;
  register int32_t d1 __asm__("d1") = value;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
#else
  (void)value;
#endif
}

static inline void easy68k_print_radix(uint32_t value, uint8_t base) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 15;
  register uint32_t d1 __asm__("d1") = value;
  register uint32_t d2 __asm__("d2") = base;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1), "+d"(d2) : : "cc", "memory");
#else
  (void)value;
  (void)base;
#endif
}

static inline int32_t easy68k_read_int(void) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 4;
  register int32_t d1 __asm__("d1") = 0;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
  return d1;
#else
  return 0;
#endif
}

static inline char easy68k_getchar(void) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 5;
  register uint32_t d1 __asm__("d1") = 0;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
  return (char)(d1 & 0xFFu);
#else
  return 0;
#endif
}

static inline void easy68k_set_cursor(uint8_t col, uint8_t row) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 11;
  register uint32_t d1 __asm__("d1") = ((uint32_t)col << 8) | row;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
#else
  (void)col;
  (void)row;
#endif
}

static inline void easy68k_clear_screen(void) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 11;
  register uint32_t d1 __asm__("d1") = 0xFF00u;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
#endif
}

/* Cycle counter & timing (Trap #15 tasks 8, 23, 30, 31). */
static inline void easy68k_clear_cycles(void) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 30;
  __asm__ volatile("trap #15" : "+d"(d0) : : "cc", "memory");
#endif
}

static inline uint32_t easy68k_get_cycles(void) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 31;
  register uint32_t d1 __asm__("d1") = 0;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
  return d1;
#else
  return 0;
#endif
}

static inline uint32_t easy68k_time_hundredths(void) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 8;
  register uint32_t d1 __asm__("d1") = 0;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
  return d1;
#else
  return 0;
#endif
}

static inline void easy68k_delay_hundredths(uint32_t hundredths) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 23;
  register uint32_t d1 __asm__("d1") = hundredths;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
#else
  (void)hundredths;
#endif
}

/* Graphics (Trap #15 tasks 80-94). */
static inline void easy68k_set_pen_color(uint32_t bgr) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 80;
  register uint32_t d1 __asm__("d1") = bgr;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
#else
  (void)bgr;
#endif
}

static inline void easy68k_set_fill_color(uint32_t bgr) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 81;
  register uint32_t d1 __asm__("d1") = bgr;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
#else
  (void)bgr;
#endif
}

static inline void easy68k_draw_pixel(int16_t x, int16_t y) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 82;
  register int16_t d1 __asm__("d1") = x;
  register int16_t d2 __asm__("d2") = y;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1), "+d"(d2) : : "cc", "memory");
#else
  (void)x;
  (void)y;
#endif
}

static inline void easy68k_draw_rect(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 87;
  register int16_t d1 __asm__("d1") = x1;
  register int16_t d2 __asm__("d2") = y1;
  register int16_t d3 __asm__("d3") = x2;
  register int16_t d4 __asm__("d4") = y2;
  __asm__ volatile("trap #15"
                   : "+d"(d0), "+d"(d1), "+d"(d2), "+d"(d3), "+d"(d4)
                   :
                   : "cc", "memory");
#else
  (void)x1;
  (void)y1;
  (void)x2;
  (void)y2;
#endif
}

static inline void easy68k_set_double_buffer(bool enable) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 92;
  register uint32_t d1 __asm__("d1") = enable ? 17u : 16u;
  __asm__ volatile("trap #15" : "+d"(d0), "+d"(d1) : : "cc", "memory");
#else
  (void)enable;
#endif
}

static inline void easy68k_repaint(void) {
#if defined(__mc68000__) || defined(__m68k__)
  register uint32_t d0 __asm__("d0") = 94;
  __asm__ volatile("trap #15" : "+d"(d0) : : "cc", "memory");
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* EASY68K_RUNTIME_H_ */
