// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Type definitions for 68000 simulator.
// Based on original code by Paul McKee and Charles Kelly.

#ifndef EASY68K_SIM_TYPES_H_
#define EASY68K_SIM_TYPES_H_

#include <cstdint>

namespace easym68k::sim {

// =============================================================================
// Memory Configuration
// =============================================================================

constexpr uint32_t kMemorySize = 0x01000000;  // 16 MB address space
constexpr uint32_t kAddressMask = 0x00FFFFFF;

// =============================================================================
// Register Configuration
// =============================================================================

constexpr int kDataRegCount = 8;  // D0-D7
constexpr int kAddrRegCount = 8;  // A0-A7
constexpr int kSspIndex = 8;      // SSP lives at state_.ssp, not in a[8]

// =============================================================================
// Data Size Masks
// =============================================================================

constexpr uint32_t kByteMask = 0x000000FF;
constexpr uint32_t kWordMask = 0x0000FFFF;
constexpr uint32_t kLongMask = 0xFFFFFFFF;

// =============================================================================
// Status Register Bits
// =============================================================================

constexpr uint16_t kSrCarry = 0x0001;       // C - Carry
constexpr uint16_t kSrOverflow = 0x0002;    // V - Overflow
constexpr uint16_t kSrZero = 0x0004;        // Z - Zero
constexpr uint16_t kSrNegative = 0x0008;    // N - Negative
constexpr uint16_t kSrExtend = 0x0010;      // X - Extend
constexpr uint16_t kSrIntMask = 0x0700;     // I2-I0 - Interrupt mask
constexpr uint16_t kSrSupervisor = 0x2000;  // S - Supervisor
constexpr uint16_t kSrTrace = 0x8000;       // T - Trace

// =============================================================================
// Enumerations
// =============================================================================

// Data size for operations
enum class DataSize { kByte = 0, kWord = 1, kLong = 2 };

constexpr uint32_t kByteSignBit = 0x80u;
constexpr uint32_t kWordSignBit = 0x8000u;
constexpr uint32_t kLongSignBit = 0x80000000u;

constexpr uint32_t SizeMask(DataSize size) {
  switch (size) {
    case DataSize::kByte:
      return kByteMask;
    case DataSize::kWord:
      return kWordMask;
    default:
      return kLongMask;
  }
}

constexpr uint32_t SignBit(DataSize size) {
  switch (size) {
    case DataSize::kByte:
      return kByteSignBit;
    case DataSize::kWord:
      return kWordSignBit;
    default:
      return kLongSignBit;
  }
}

// Condition codes for Bcc, DBcc, Scc
enum class Condition {
  kTrue = 0x00,            // T
  kFalse = 0x01,           // F
  kHigh = 0x02,            // HI
  kLowOrSame = 0x03,       // LS
  kCarryClear = 0x04,      // CC/HS
  kCarrySet = 0x05,        // CS/LO
  kNotEqual = 0x06,        // NE
  kEqual = 0x07,           // EQ
  kOverflowClear = 0x08,   // VC
  kOverflowSet = 0x09,     // VS
  kPlus = 0x0A,            // PL
  kMinus = 0x0B,           // MI
  kGreaterOrEqual = 0x0C,  // GE
  kLessThan = 0x0D,        // LT
  kGreaterThan = 0x0E,     // GT
  kLessOrEqual = 0x0F      // LE
};

// Simulator execution result codes — no exceptions, returned from instruction dispatch
enum class SimResult : uint8_t {
  kOk = 0,
  kBadInstruction,
  kPrivilegeViolation,
  kAddressError,
  kBusError,
  kDivideByZero,
  kChkException,
  kTrapVException,
  kTraceException,
  kLine1010,
  kLine1111,
  kStopInstruction,
  kTrapException,
  kHalted
};

// Exception vector numbers
enum class ExceptionVector {
  kResetSsp = 0,
  kResetPc = 1,
  kBusError = 2,
  kAddressError = 3,
  kIllegalInst = 4,
  kDivideByZero = 5,
  kChk = 6,
  kTrapV = 7,
  kPrivilege = 8,
  kTrace = 9,
  kLine1010 = 10,
  kLine1111 = 11,
  kSpuriousInt = 24,
  kAutovector1 = 25,
  kAutovector2 = 26,
  kAutovector3 = 27,
  kAutovector4 = 28,
  kAutovector5 = 29,
  kAutovector6 = 30,
  kAutovector7 = 31,
  kTrap0 = 32,
  kTrap15 = 47
};

// Memory protection flags
enum MemoryFlags : uint8_t {
  kMemNormal = 0x00,
  kMemInvalid = 0x01,
  kMemProtected = 0x02,
  kMemReadLog = 0x04,
  kMemRom = 0x10
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_TYPES_H_
