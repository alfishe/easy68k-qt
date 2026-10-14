// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Branch and jump instructions: BRA, BSR, Bcc, DBcc, Scc, JMP, JSR, RTS,
// RTE, RTR.
// Ported from EASy68K/Sim68K/CODE6.CPP and CODE7.CPP.

#include "easym68k/sim/addressing_mode.h"
#include "easym68k/sim/cycle_data.h"
#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

// ---------------------------------------------------------------------------
// JMP/JSR execution time by EaCode() (CODE8.CPP JMP()/JSR(); eff_addr() is
// called with add_times=false, so the table includes the EA time).
// ---------------------------------------------------------------------------

static int JmpJsrCycles(int ea_code, bool is_jsr) {
  int jmp;
  switch (ea_code) {
    case 0x02:  // (An)
      jmp = 8;
      break;
    case 0x05:  // d(An)
    case 0x07:  // xxx.W
    case 0x09:  // d(PC)
      jmp = 10;
      break;
    case 0x08:  // xxx.L
      jmp = 12;
      break;
    case 0x06:  // d(An,Xi)
    case 0x0A:  // d(PC,Xi)
      jmp = 14;
      break;
    default:
      return 0;
  }
  return is_jsr ? jmp + 8 : jmp;
}

// ---------------------------------------------------------------------------
// Branch displacement helper.
// The branch displacement is encoded as:
//   - 8-bit displacement in opcode byte (non-zero → short form)
//   - 0x00 → word displacement follows in next instruction word
// Displacement is relative to the address of the word AFTER the opcode.
// ---------------------------------------------------------------------------

static int32_t FetchBranchDisp(uint16_t opcode, M68kSimulator& sim) {
  int8_t disp8 = static_cast<int8_t>(opcode & 0xFF);
  if (disp8 != 0)
    return static_cast<int32_t>(disp8);
  return static_cast<int32_t>(static_cast<int16_t>(sim.FetchWord()));
}

// ---------------------------------------------------------------------------
// BRA — unconditional branch
// Opcode: 0110 0000 dddddddd
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpBra(uint16_t opcode) {
  uint32_t base = state_.pc;  // pc already past opcode word
  int32_t disp = FetchBranchDisp(opcode, *this);
  state_.pc = base + disp;  // 32-bit PC (CODE8.CPP: PC = OLD_PC + disp + 2)
  IncrementCycles(10);      // CODE8.CPP BRA
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// BSR — branch to subroutine (push return address, then branch)
// Opcode: 0110 0001 dddddddd
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpBsr(uint16_t opcode) {
  uint32_t base = state_.pc;
  int32_t disp = FetchBranchDisp(opcode, *this);
  // Push return address (address after the full BSR instruction)
  state_.SetSP(state_.GetSP() - 4);
  memory_.WriteLong(state_.GetSP() & kAddressMask, state_.pc);
  NoteSubroutineCall(state_.pc);
  state_.pc = base + disp;
  IncrementCycles(18);  // CODE8.CPP BSR
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// Bcc — branch conditionally
// Opcode: 0110 cccc dddddddd   cccc = condition (2-15; 0=BRA, 1=BSR)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpBcc(uint16_t opcode) {
  uint32_t base = state_.pc;
  int32_t disp = FetchBranchDisp(opcode, *this);
  uint8_t cond = (opcode >> 8) & 0xF;
  if (CheckCondition(static_cast<Condition>(cond))) {
    state_.pc = base + disp;
    IncrementCycles(10);  // CODE8.CPP BCC: taken
  } else {
    IncrementCycles((opcode & 0xFF) ? 8 : 12);  // not taken: short 8, word 12
  }
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// DBcc — decrement and branch
// Opcode: 0101 cccc 1100 1DDD, followed by 16-bit displacement word
// If condition is false: Dn.W-- ; if Dn.W != -1, branch to pc + disp
// If condition is true or Dn.W reaches -1: fall through (pc past disp word)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpDbcc(uint16_t opcode) {
  uint8_t cond = (opcode >> 8) & 0xF;
  int dn = opcode & 0x7;
  uint32_t disp_pc = state_.pc;  // pc points at displacement word
  int16_t disp = static_cast<int16_t>(FetchWord());

  // CODE8.CPP DBCC: cond true 12; counter expired 14; branch taken 10.
  if (!CheckCondition(static_cast<Condition>(cond))) {
    int16_t counter = static_cast<int16_t>(state_.d[dn] & 0xFFFF);
    --counter;
    state_.d[dn] = (state_.d[dn] & 0xFFFF0000u) | static_cast<uint16_t>(counter);
    if (counter != -1) {
      state_.pc = disp_pc + disp;
      IncrementCycles(10);
    } else {
      IncrementCycles(14);
    }
  } else {
    IncrementCycles(12);
  }
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// Scc — set byte according to condition
// Opcode: 0101 cccc 11 MMMRRR
// Sets EA byte to 0xFF if condition true, 0x00 if false
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpScc(uint16_t opcode) {
  uint8_t cond = (opcode >> 8) & 0xF;
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  uint8_t val = CheckCondition(static_cast<Condition>(cond)) ? 0xFF : 0x00;
  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kByte);
  // CODE8.CPP SCC: EA time + Dn: 6 (true) / 4 (false); memory: 8.
  IncrementCycles(EaCyclePenalty(ea, DataSize::kByte));
  if ((opcode & 0x0030) == 0)
    IncrementCycles(val ? 6 : 4);
  else
    IncrementCycles(8);
  // The 68000 reads the destination before writing it (CLR, Scc and MOVE
  // from SR are read-modify-write cycles), as the original does via
  // eff_addr(): an odd address faults as a read.
  (void)ReadEA(ea, DataSize::kByte);
  return WriteEA(ea, val, DataSize::kByte);
}

// ---------------------------------------------------------------------------
// JMP — jump to effective address (no stack frame)
// Opcode: 0100 1110 11 MMMRRR
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpJmp(uint16_t opcode) {
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kLong);
  state_.pc = ea.address & kAddressMask;
  IncrementCycles(JmpJsrCycles(EaCode(mode, reg), /*is_jsr=*/false));
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// JSR — jump to subroutine (push return address, then JMP)
// Opcode: 0100 1110 10 MMMRRR
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpJsr(uint16_t opcode) {
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kLong);
  state_.SetSP(state_.GetSP() - 4);
  memory_.WriteLong(state_.GetSP() & kAddressMask, state_.pc);
  NoteSubroutineCall(state_.pc);
  state_.pc = ea.address & kAddressMask;
  IncrementCycles(JmpJsrCycles(EaCode(mode, reg), /*is_jsr=*/true));
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// RTS — return from subroutine: pop PC from stack
// Opcode: 0100 1110 0111 0101 (0x4E75)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpRts(uint16_t /*opcode*/) {
  uint32_t sp = state_.GetSP();
  auto result = memory_.ReadLong(sp & kAddressMask);
  state_.pc = result.value;  // all 32 bits popped (the 68000 PC is 32-bit)
  state_.SetSP(sp + 4);
  IncrementCycles(16);  // CODE8.CPP RTS
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// RTE — return from exception: pop SR then PC from stack (supervisor only)
// Opcode: 0100 1110 0111 0011 (0x4E73)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpRte(uint16_t /*opcode*/) {
  if (!state_.IsSupervisor())
    return SimResult::kPrivilegeViolation;
  uint32_t sp = state_.ssp;
  auto sr_res = memory_.ReadWord(sp & kAddressMask);
  auto pc_res = memory_.ReadLong((sp + 2) & kAddressMask);
  state_.sr = static_cast<uint16_t>(sr_res.value);
  state_.pc = pc_res.value;
  state_.ssp = sp + 6;
  IncrementCycles(20);  // CODE8.CPP RTE
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// RTR — return and restore CCR: pop CCR then PC from stack
// Opcode: 0100 1110 0111 0111 (0x4E77)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpRtr(uint16_t /*opcode*/) {
  uint32_t sp = state_.GetSP();
  auto ccr_res = memory_.ReadWord(sp & kAddressMask);
  auto pc_res = memory_.ReadLong((sp + 2) & kAddressMask);
  // Restore only CCR (low byte), not the supervisor half
  state_.sr = static_cast<uint16_t>((state_.sr & 0xFF00u) | (ccr_res.value & 0xFFu));
  state_.pc = pc_res.value;
  state_.SetSP(sp + 6);
  IncrementCycles(20);  // CODE8.CPP RTR
  return SimResult::kOk;
}

}  // namespace easym68k::sim
