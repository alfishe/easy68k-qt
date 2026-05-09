// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// Phase 6 stub — misc opcodes not yet implemented.

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

SimResult M68kSimulator::OpTrap(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpTrapV(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpIllegal(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpReset(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpStop(uint16_t) {
  return SimResult::kBadInstruction;
}

}  // namespace easym68k::sim
