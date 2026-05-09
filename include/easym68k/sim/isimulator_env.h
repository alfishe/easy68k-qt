// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ISimulatorEnv — simulator environment interface (Trap #15 tasks 8, 23,
// 30-33). Provides timers, cycle counter, version, exception control, hardware
// window, output window sizing, and memory-mapped peripheral addresses.
// Replaces Win32 GetTickCount and VCL hardware/output windows in original.

#pragma once

#include <cstdint>

namespace easym68k::sim {

class ISimulatorEnv {
 public:
  virtual ~ISimulatorEnv() = default;
  virtual uint32_t GetTimeHundredths() = 0;
  virtual void DelayHundredths(unsigned int n) = 0;
  virtual void ShowHardware() = 0;
  virtual uint32_t GetSeg7Address() = 0;
  virtual uint32_t GetLEDAddress() = 0;
  virtual uint32_t GetSwitchAddress() = 0;
  virtual uint32_t GetPushButtonAddress() = 0;
  virtual void SetAutoIRQ(uint8_t irq_config, uint32_t interval) = 0;
  virtual void ClearCycles() = 0;
  virtual uint32_t GetCycles() = 0;
  virtual uint32_t GetVersion() = 0;
  virtual void SetExceptionsEnabled(bool enable) = 0;
  virtual void SetWindowSize(unsigned short width, unsigned short height) = 0;
  virtual void GetWindowSize(unsigned short* width, unsigned short* height) = 0;
  virtual void SetupWindow(bool fullscreen) = 0;
};

}  // namespace easym68k::sim
