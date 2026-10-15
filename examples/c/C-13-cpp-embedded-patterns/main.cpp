/*
 * C-13: Embedded C++17 — Placement new, Static Allocation, and CRTP Drivers
 * SPDX-License-Identifier: CC0-1.0
 */
#include "../runtime/easy68k.h"

/* Freestanding placement new */
inline void* operator new(size_t, void* p) noexcept { return p; }

/* CRTP base class: compile-time polymorphism without a vtable */
template <typename Derived>
struct HwDriverBase {
  void write_status(uint8_t v) { static_cast<Derived*>(this)->write_impl(v); }
};

struct LedDriver : HwDriverBase<LedDriver> {
  void write_impl(uint8_t v) { *EASY68K_LED_ADDR = v; }
};

struct Seg7Driver : HwDriverBase<Seg7Driver> {
  void write_impl(uint8_t v) { EASY68K_SEG7_ADDR[0] = v; }
};

struct DeviceState {
  uint32_t magic;
  uint8_t  led_val;
  uint8_t  seg7_val;
  DeviceState(uint32_t m, uint8_t led, uint8_t seg7)
      : magic(m), led_val(led), seg7_val(seg7) {}
};

int main() {
  void* storage = (void*)0x2000UL;
  DeviceState* dev = new (storage) DeviceState(0x68000C13UL, 0xC3, 0x07);

  Seg7Driver seg7;
  LedDriver  leds;
  seg7.write_status(dev->seg7_val); /* $07 ('7') -> $E00000 */
  leds.write_status(dev->led_val);  /* $C3       -> $E00010 */
  return (int)dev->led_val;         /* 195 ($C3) */
}
