/*
 * C-11: C++ Virtual Dispatch and Vtables on the 68000
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

struct Shape {
  virtual int32_t area() const = 0;
};

struct Rect : Shape {
  int16_t w;
  int16_t h;
  Rect(int16_t width, int16_t height) : w(width), h(height) {}
  int32_t area() const override { return (int32_t)w * (int32_t)h; }
};

struct Square : Shape {
  int16_t side;
  int16_t pad;
  explicit Square(int16_t s) : side(s), pad(0) {}
  int32_t area() const override { return (int32_t)side * (int32_t)side; }
};

static int32_t call_area(const Shape* s) {
  return s->area(); /* virtual dispatch via vptr at offset 0 */
}

int main() {
  Rect r(6, 7);   /* area = 42 */
  Square sq(5);   /* area = 25 */
  return (int)(call_area(&r) + call_area(&sq)); /* 67 */
}
