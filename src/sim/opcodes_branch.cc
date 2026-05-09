// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// Phase 6 stub — branch opcodes not yet implemented.

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

SimResult M68kSimulator::OpBra(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpBsr(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpBcc(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpDbcc(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpScc(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpJmp(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpJsr(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpRts(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpRte(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpRtr(uint16_t) {
  return SimResult::kBadInstruction;
}

}  // namespace easym68k::sim
