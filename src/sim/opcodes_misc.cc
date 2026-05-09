// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Miscellaneous instructions: TRAP, TRAPV, ILLEGAL, RESET, STOP.
// Ported from EASy68K/Sim68K/CODE9.CPP.

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

// ---------------------------------------------------------------------------
// TRAP #n — generate trap exception (vector kTrap0 + n)
// Opcode: 0100 1110 0100 nnnn  n = 0..15
// Step() extracts n from (opcode & 0x0F) when handling kTrapException.
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpTrap(uint16_t /*opcode*/) {
  return SimResult::kTrapException;
}

// ---------------------------------------------------------------------------
// TRAPV — trap on overflow
// Opcode: 0100 1110 0111 0110 (0x4E76)
// If V set, generate TRAPV exception; otherwise no-op.
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpTrapV(uint16_t /*opcode*/) {
  if (state_.GetFlag(kSrOverflow))
    return SimResult::kTrapVException;
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// ILLEGAL — always generate illegal instruction exception
// Opcode: 0100 1010 1111 1100 (0x4AFC)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpIllegal(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// ---------------------------------------------------------------------------
// RESET — assert external /RESET line (supervisor only; no-op in simulation)
// Opcode: 0100 1110 0111 0000 (0x4E70)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpReset(uint16_t /*opcode*/) {
  if (!state_.IsSupervisor())
    return SimResult::kPrivilegeViolation;
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// STOP — load immediate into SR, then halt until interrupt (supervisor only)
// Opcode: 0100 1110 0111 0010 (0x4E72), followed by 16-bit immediate
//
// Matches CODE9.CPP order:
//   1. Fetch immediate word (before privilege check, so PC always advances).
//   2. Pre-load privilege check.
//   3. Load SR, preserving trace flag if it was active.
//   4. Post-load privilege check (new SR may clear supervisor bit).
//   5. If trace was on, generate trace exception instead of halting.
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpStop(uint16_t /*opcode*/) {
  uint16_t new_sr = FetchWord();  // always fetched, even on privilege violation

  if (!state_.IsSupervisor())
    return SimResult::kPrivilegeViolation;

  bool trace_was_on = state_.GetFlag(kSrTrace);
  state_.sr = new_sr;
  if (trace_was_on)
    state_.sr |= kSrTrace;  // preserve active trace across SR load

  if (!state_.IsSupervisor())  // new SR may have cleared S bit
    return SimResult::kPrivilegeViolation;

  if (trace_was_on)
    return SimResult::kTraceException;

  return SimResult::kStopInstruction;
}

}  // namespace easym68k::sim
