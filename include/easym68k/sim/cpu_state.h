// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// CpuState — 68000 register file and status register.

#ifndef EASY68K_SIM_CPU_STATE_H_
#define EASY68K_SIM_CPU_STATE_H_

#include <cstdint>

#include "easym68k/sim/types.h"

namespace easym68k::sim {

struct CpuState {
  uint32_t d[8];  // Data registers D0-D7
  uint32_t a[8];  // Address registers A0-A7 (A7 = USP in user mode)
  uint32_t usp;   // User Stack Pointer (shadows a[7] in user mode)
  uint32_t ssp;   // Supervisor Stack Pointer (active when S bit set)
  uint32_t pc;
  uint16_t sr;
  uint64_t cycles;
  bool halted;
  bool stopped;

  bool IsSupervisor() const { return (sr & kSrSupervisor) != 0; }

  // Returns the value of address register reg (0-7).
  // When in supervisor mode and reg==7, returns ssp instead of a[7].
  uint32_t GetAReg(int reg) const {
    if (reg == 7 && IsSupervisor())
      return ssp;
    return a[reg];
  }

  // Sets address register reg.  Supervisor/A7 → ssp; otherwise → a[reg].
  void SetAReg(int reg, uint32_t value) {
    if (reg == 7 && IsSupervisor())
      ssp = value;
    else
      a[reg] = value;
  }

  uint32_t GetSP() const { return IsSupervisor() ? ssp : a[7]; }
  void SetSP(uint32_t value) {
    if (IsSupervisor())
      ssp = value;
    else
      a[7] = value;
  }

  bool GetFlag(uint16_t flag) const { return (sr & flag) != 0; }
  void SetFlag(uint16_t flag, bool value) {
    if (value)
      sr |= flag;
    else
      sr &= static_cast<uint16_t>(~flag);
  }

  int GetInterruptMask() const { return (sr & kSrIntMask) >> 8; }

  void SetSupervisor(bool value) { SetFlag(kSrSupervisor, value); }

  void SetInterruptMask(int level) {
    sr = static_cast<uint16_t>((sr & ~kSrIntMask) | ((level & 0x7) << 8));
  }
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_CPU_STATE_H_
