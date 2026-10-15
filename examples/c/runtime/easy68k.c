/*
 * libeasy68k — Freestanding support routines (memcpy, memset, strlen) for 68000.
 * SPDX-License-Identifier: CC0-1.0
 */
#include "easy68k.h"

void* memcpy(void* dst, const void* src, size_t n) {
  uint8_t* d = (uint8_t*)dst;
  const uint8_t* s = (const uint8_t*)src;
  while (n--) {
    *d++ = *s++;
  }
  return dst;
}

void* memset(void* dst, int c, size_t n) {
  uint8_t* d = (uint8_t*)dst;
  while (n--) {
    *d++ = (uint8_t)c;
  }
  return dst;
}

size_t strlen(const char* s) {
  const char* p = s;
  while (*p) {
    ++p;
  }
  return (size_t)(p - s);
}
