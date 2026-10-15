/*
 * C-10: Freestanding C++17 — Classes, Constructors, this Pointer, and RAII
 * Compile with: m68k-linux-gnu-g++ -m68000 -std=c++17 -O2 -ffreestanding -fno-exceptions -fno-rtti
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

class ScopedLed {
 public:
  ScopedLed(uint8_t active_val, uint8_t exit_val) : exit_val_(exit_val) {
    *EASY68K_LED_ADDR = active_val;
  }
  ~ScopedLed() {
    *EASY68K_LED_ADDR = exit_val_;
  }
 private:
  uint8_t exit_val_;
};

class Vec2D {
 public:
  Vec2D(int16_t x, int16_t y) : x_(x), y_(y) {}
  int32_t manhattan() const { return (int32_t)x_ + (int32_t)y_; }
  int32_t dot(const Vec2D& other) const {
    return (int32_t)x_ * other.x_ + (int32_t)y_ * other.y_;
  }
 private:
  int16_t x_;
  int16_t y_;
};

int main() {
  ScopedLed guard(0xA5, 0x5A);
  Vec2D a(12, 5);
  Vec2D b(3, 4);
  Vec2D c(5, 6);
  return (int)(a.manhattan() + b.dot(c)); /* 17 + (15 + 24) = 56 */
}
