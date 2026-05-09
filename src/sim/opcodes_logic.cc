// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// Phase 6 stub — logic opcodes not yet implemented.

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

SimResult M68kSimulator::OpOr(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpOri(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpOriToCcr(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpOriToSr(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpAnd(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpAndi(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpAndiToCcr(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpAndiToSr(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpEor(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpEori(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpEoriToCcr(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpEoriToSr(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpNot(uint16_t) {
  return SimResult::kBadInstruction;
}
SimResult M68kSimulator::OpTas(uint16_t) {
  return SimResult::kBadInstruction;
}

}  // namespace easym68k::sim
