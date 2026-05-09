// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// Phase 6 stub — shift/rotate and bit opcodes not yet implemented.

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

SimResult M68kSimulator::OpShiftRotate(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpBtst(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpBchg(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpBclr(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpBset(uint16_t) {
  return SimResult::kBadInstruction;
}

}  // namespace easym68k::sim
