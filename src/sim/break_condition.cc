// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Advanced breakpoint evaluation — ported from Sim68K BPoint::isBreak(),
// BPointExpr::isBreak() and the runprog() loop (RUN.CPP).

#include <utility>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

void M68kSimulator::SetBreakCondition(int index, const BreakCondition& condition) {
  if (index >= 0 && index < kMaxBreakConditions)
    break_conditions_[index] = condition;
}

const BreakCondition& M68kSimulator::GetBreakCondition(int index) const {
  static const BreakCondition kNone;
  return (index >= 0 && index < kMaxBreakConditions) ? break_conditions_[index] : kNone;
}

void M68kSimulator::SetBreakExpressions(std::vector<BreakExpression> expressions) {
  if (expressions.size() > static_cast<size_t>(kMaxBreakExpressions))
    expressions.resize(kMaxBreakExpressions);
  break_expressions_ = std::move(expressions);
  break_pass_counts_.assign(break_expressions_.size(), 0);
  advanced_break_hit_ = false;
}

void M68kSimulator::ClearAdvancedBreakpoints() {
  for (BreakCondition& c : break_conditions_)
    c = BreakCondition{};
  SetBreakExpressions({});
}

// BPoint::isBreak()
bool M68kSimulator::IsConditionMet(const BreakCondition& c) {
  if (!c.enabled)  // Is breakpoint valid to check?
    return false;

  bool final_condition = true;
  if (c.type == BreakCondition::kAddress) {
    // Is the readWrite condition met? (readEA/writeEA: the last access)
    const Memory::AccessTracking& t = memory_.GetAccessTracking();
    const uint32_t addr = c.type_id & kAddressMask;
    bool read = false;
    bool write = false;
    if (t.read && t.read_address == addr)
      read = true;
    else if (t.write && t.write_address == addr)
      write = true;
    switch (c.access) {
      case BreakCondition::kReadWrite:
        final_condition = read || write;
        break;
      case BreakCondition::kRead:
        final_condition = read;
        break;
      case BreakCondition::kWrite:
        final_condition = write;
        break;
      case BreakCondition::kAnyAccess:  // We don't care if currently reading or writing
        final_condition = true;
        break;
      default:  // Invalid readWrite type specified.  No break.
        final_condition = false;
        break;
    }
  }
  if (!final_condition)
    return false;

  // Is the value in the correct range? (the original leaves the size mask
  // undefined for an invalid size; the port uses a long)
  uint32_t mask = c.size == BreakCondition::kByte   ? 0xFFu
                  : c.size == BreakCondition::kWord ? 0xFFFFu
                                                    : 0xFFFFFFFFu;
  uint32_t found = 0;
  if (c.type == BreakCondition::kPcReg && c.type_id == BreakCondition::kPc) {
    found = state_.pc & mask;
  } else if (c.type == BreakCondition::kPcReg) {
    const uint32_t id = c.type_id;
    if (id < 8)
      found = state_.d[id] & mask;
    else if (id < 16)
      found = state_.a[id - 8] & mask;  // A7 is the user stack pointer (A[7])
  } else {
    // value_of() reads the memory location through mem_req(), which also
    // records it as the last read (as in the original). A failed read leaves
    // the value 0 and no fault behind.
    const bool had_fault = memory_.HasFault();
    const uint32_t addr = c.type_id & kAddressMask;
    MemoryAccessResult r = c.size == BreakCondition::kByte   ? memory_.ReadByte(addr)
                           : c.size == BreakCondition::kWord ? memory_.ReadWord(addr)
                                                             : memory_.ReadLong(addr);
    if (!had_fault && memory_.HasFault())
      memory_.ClearFault();
    found = r.success ? (r.value & mask) : 0;
  }

  // Signed comparison of the original's longs.
  const int32_t value_found = static_cast<int32_t>(found);
  switch (c.op) {
    case BreakCondition::kEqual:
      return value_found == c.value;
    case BreakCondition::kNotEqual:
      return value_found != c.value;
    case BreakCondition::kGreater:
      return value_found > c.value;
    case BreakCondition::kGreaterEqual:
      return value_found >= c.value;
    case BreakCondition::kLess:
      return value_found < c.value;
    case BreakCondition::kLessEqual:
      return value_found <= c.value;
    case BreakCondition::kAnyValue:  // Value does not matter.
      return true;
    default:  // Invalid op type specified.  No break.
      return false;
  }
}

// BPointExpr::isBreak(): postfix evaluation and pass count.
bool M68kSimulator::IsExpressionBreak(int index) {
  const BreakExpression& e = break_expressions_[index];
  if (!e.enabled)
    return false;

  std::vector<bool> operands;
  for (int token : e.postfix) {
    if (token >= 0 && token < kMaxBreakConditions) {
      operands.push_back(IsConditionMet(break_conditions_[token]));
    } else {
      // The original pops two operands (undefined if missing); a malformed
      // expression does not break here.
      if (operands.size() < 2)
        return false;
      bool a = operands.back();
      operands.pop_back();
      bool b = operands.back();
      operands.pop_back();
      bool result = false;
      if (token == BreakExpression::kAnd)
        result = a && b;
      else if (token == BreakExpression::kOr)
        result = a || b;
      operands.push_back(result);
    }
  }
  if (operands.empty())
    return false;
  bool brk = operands.back();

  // Verify that the condition has been met the specified number of times.
  int& counter = break_pass_counts_[index];
  if (brk)
    counter++;
  brk = (e.count == counter);
  if (brk)
    counter = 0;  // Reset counter for next iteration.
  return brk;
}

bool M68kSimulator::EvaluateBreakExpressions() {
  // runprog(): every expression is evaluated (no short circuit), so all pass
  // counters advance.
  bool hit = false;
  for (int i = 0; i < static_cast<int>(break_expressions_.size()); ++i)
    if (IsExpressionBreak(i))
      hit = true;
  return hit;
}

}  // namespace easym68k::sim
