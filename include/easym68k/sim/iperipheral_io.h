// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// IPeripheralIO — mouse and keyboard IRQ interface (Trap #15 tasks 60-62).
// Replaces Win32 mouse/keyboard hook APIs in original CODE9.CPP.

#pragma once

#include <cstdint>

namespace easym68k::sim {

class IPeripheralIO {
 public:
  virtual ~IPeripheralIO() = default;
  virtual void SetMouseIRQ(uint8_t irq_level, uint8_t events) = 0;
  virtual void ReadMouse(int mode, long* button_state, long* position) = 0;
  virtual void SetKeyIRQ(uint8_t irq_level, uint8_t events) = 0;
};

}  // namespace easym68k::sim
