// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Instruction cycle timing data.
// Ported from EASy68K/Sim68K/RUN.CPP eff_addr(), CODE1.CPP timing tables and
// UTILS.CPP eff_addr_code().

#include "easym68k/sim/cycle_data.h"

namespace easym68k::sim {

namespace {

// CODE1.CPP move_bw_times[12][9] — MOVE.B / MOVE.W execution times.
// Rows: source EaCode (0-11). Columns: destination EaCode (0-8).
constexpr int kMoveBwTimes[12][9] = {
    {4, 4, 8, 8, 8, 12, 14, 12, 16},      {4, 4, 8, 8, 8, 12, 14, 12, 16},
    {8, 8, 12, 12, 12, 16, 18, 16, 20},   {8, 8, 12, 12, 12, 16, 18, 16, 20},
    {10, 10, 14, 14, 14, 18, 20, 18, 22}, {12, 12, 16, 16, 16, 20, 22, 20, 24},
    {14, 14, 18, 18, 18, 22, 24, 22, 26}, {12, 12, 16, 16, 16, 20, 22, 20, 24},
    {16, 16, 20, 20, 20, 24, 26, 24, 28}, {12, 12, 16, 16, 16, 20, 22, 20, 24},
    {14, 14, 18, 18, 18, 22, 24, 22, 26}, {8, 8, 12, 12, 12, 16, 18, 16, 20},
};

// CODE1.CPP move_l_times[12][9] — MOVE.L execution times.
constexpr int kMoveLTimes[12][9] = {
    {4, 4, 12, 12, 12, 16, 18, 16, 20},   {4, 4, 12, 12, 12, 16, 18, 16, 20},
    {12, 12, 20, 20, 20, 24, 26, 24, 28}, {12, 12, 20, 20, 20, 24, 26, 24, 28},
    {14, 14, 22, 22, 22, 26, 28, 26, 30}, {16, 16, 24, 24, 24, 28, 30, 28, 32},
    {18, 18, 26, 26, 26, 30, 32, 30, 34}, {16, 16, 24, 24, 24, 28, 30, 28, 32},
    {20, 20, 28, 28, 28, 32, 34, 32, 36}, {16, 16, 24, 24, 24, 28, 30, 28, 32},
    {18, 18, 26, 26, 26, 30, 32, 30, 34}, {12, 12, 20, 20, 20, 24, 26, 24, 28},
};

// CODE1.CPP movem_t_r_times[11] — MOVEM memory-to-registers base times.
constexpr int kMovemToRegTimes[11] = {0, 0, 12, 12, 0, 16, 18, 16, 20, 16, 18};

// CODE1.CPP mover_t_m_times[11] — MOVEM registers-to-memory base times.
constexpr int kMovemToMemTimes[11] = {0, 0, 8, 0, 8, 12, 14, 12, 16, 0, 0};

}  // namespace

int EaCyclePenalty(int mode, int reg, DataSize size) {
  int bwinc = 0;
  int linc = 0;
  switch (mode) {
    case 0:  // Dn
    case 1:  // An
      break;
    case 2:  // (An)
    case 3:  // (An)+
      bwinc = 4;
      linc = 8;
      break;
    case 4:  // -(An)
      bwinc = 6;
      linc = 10;
      break;
    case 5:  // d(An)
      bwinc = 8;
      linc = 12;
      break;
    case 6:  // d(An,Xi)
      bwinc = 10;
      linc = 14;
      break;
    case 7:
      switch (reg) {
        case 0:  // xxx.W
          bwinc = 8;
          linc = 12;
          break;
        case 1:  // xxx.L
          bwinc = 12;
          linc = 16;
          break;
        case 2:  // d(PC)
          bwinc = 8;
          linc = 12;
          break;
        case 3:  // d(PC,Xi)
          bwinc = 10;
          linc = 14;
          break;
        case 4:  // #imm
          bwinc = 4;
          linc = 8;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return (size == DataSize::kLong) ? linc : bwinc;
}

int EaCode(int mode, int reg) {
  if (mode != 7)
    return mode;
  if (reg >= 0 && reg <= 4)
    return 7 + reg;
  return 12;
}

int MoveCycles(int src_code, int dst_code, DataSize size) {
  if (src_code < 0 || src_code >= 12 || dst_code < 0 || dst_code >= 9)
    return 0;
  return (size == DataSize::kLong) ? kMoveLTimes[src_code][dst_code]
                                   : kMoveBwTimes[src_code][dst_code];
}

int MovemCycles(int ea_code, bool memory_to_register) {
  if (ea_code < 0 || ea_code >= 11)
    return 0;
  return memory_to_register ? kMovemToRegTimes[ea_code] : kMovemToMemTimes[ea_code];
}

int MuluCycles(uint16_t source) {
  int n = 0;
  for (int i = 0; i < 16; i++) {
    if ((source >> i) & 1)
      n++;
  }
  return 38 + 2 * n;
}

int MulsCycles(uint16_t source) {
  // The implicit bit to the right of the LSB is 0, so `prev` starts at 0.
  int n = 0;
  int prev = 0;
  for (int i = 0; i < 16; i++) {
    int bit = (source >> i) & 1;
    if (bit != prev) {
      n++;
      prev = bit;
    }
  }
  return 38 + 2 * n;
}

int DivuCycles(uint32_t dividend, uint16_t divisor) {
  if (divisor == 0)
    return 0;
  if ((dividend >> 16) >= divisor)  // overflow
    return 10;

  int mcycles = 38;
  uint32_t hdivisor = static_cast<uint32_t>(divisor) << 16;
  for (int i = 0; i < 15; i++) {
    uint32_t temp = dividend;
    dividend <<= 1;
    if (static_cast<int32_t>(temp) < 0) {  // carry out of the shift
      dividend -= hdivisor;
    } else {
      mcycles += 2;
      if (dividend >= hdivisor) {
        dividend -= hdivisor;
        mcycles--;
      }
    }
  }
  return mcycles * 2;
}

int DivsCycles(int32_t dividend, int16_t divisor) {
  if (divisor == 0)
    return 0;

  int mcycles = 6;
  if (dividend < 0)
    mcycles++;

  // 64-bit absolute values avoid overflow for INT32_MIN / INT16_MIN.
  uint32_t abs_dividend =
      static_cast<uint32_t>(dividend < 0 ? -static_cast<int64_t>(dividend) : dividend);
  uint32_t abs_divisor =
      static_cast<uint32_t>(divisor < 0 ? -static_cast<int32_t>(divisor) : divisor);

  if ((abs_dividend >> 16) >= abs_divisor)  // absolute overflow
    return (mcycles + 2) * 2;

  uint32_t aquot = abs_dividend / static_cast<uint16_t>(abs_divisor);
  mcycles += 55;
  if (divisor >= 0) {
    if (dividend >= 0)
      mcycles--;
    else
      mcycles++;
  }

  // Count zero bits among the 15 MSBs of the absolute quotient's low word.
  for (int i = 0; i < 15; i++) {
    if (static_cast<int16_t>(aquot & 0xFFFF) >= 0)
      mcycles++;
    aquot <<= 1;
  }
  return mcycles * 2;
}

}  // namespace easym68k::sim
