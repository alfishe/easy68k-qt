// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Advanced breakpoints (Sim68K View > Break Points): register/PC and memory
// conditions combined into AND/OR expressions with a pass count — ported
// from Sim68K BPoint.h/.cpp, BPointExpr.h/.cpp and def.h.

#ifndef EASYM68K_SIM_BREAK_CONDITION_H_
#define EASYM68K_SIM_BREAK_CONDITION_H_

#include <cstdint>
#include <string>
#include <vector>

namespace easym68k::sim {

inline constexpr int kMaxBreakConditions = 100;  // MAX_BPOINTS
inline constexpr int kMaxBreakExpressions = 50;  // MAX_BP_EXPR

// One break condition (BPoint). Register conditions use ids 0-15 (D0-D7,
// A0-A7; A7 is the user stack pointer) and 16 (PC); memory conditions use
// the address as id.
struct BreakCondition {
  enum Type : int { kPcReg = 0, kAddress = 1 };       // PC_REG_TYPE, ADDR_TYPE
  enum Id : uint32_t { kD0 = 0, kA0 = 8, kPc = 16 };  // D0_TYPE_ID .. PC_TYPE_ID
  enum Op : int {                                     // EQUAL_OP .. NA_OP
    kEqual = 0,
    kNotEqual = 1,
    kGreater = 2,
    kGreaterEqual = 3,
    kLess = 4,
    kLessEqual = 5,
    kAnyValue = 6
  };
  enum Access : int { kReadWrite = 0, kRead = 1, kWrite = 2, kAnyAccess = 3 };  // RW_TYPE..
  enum Size : int { kByte = 1, kWord = 2, kLong = 4 };

  bool enabled = false;
  int type = kPcReg;
  uint32_t type_id = kPc;
  int op = kEqual;
  int32_t value = 0;
  int access = kReadWrite;  // memory conditions only
  int size = kWord;
};

// A break expression (BPointExpr): condition indices and operators in
// postfix order, breaking when the expression has been true `count` times.
struct BreakExpression {
  // Operator tokens (MAX_BPOINTS + AND_OP / OR_OP).
  static constexpr int kAnd = kMaxBreakConditions + 0;
  static constexpr int kOr = kMaxBreakConditions + 1;

  bool enabled = false;
  std::vector<int> postfix;  // condition indices (0-99), kAnd, kOr
  int count = 1;
  std::string text;  // expression as displayed
};

}  // namespace easym68k::sim

#endif  // EASYM68K_SIM_BREAK_CONDITION_H_
