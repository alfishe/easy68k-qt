// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// M68kSimulator — 68000 simulator core.
// Ported from EASy68K/Sim68K/RUN.CPP: runprog(), exec_inst(), exceptionHandler().

#ifndef EASY68K_SIM_SIMULATOR_H_
#define EASY68K_SIM_SIMULATOR_H_

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "easym68k/sim/cpu_state.h"
#include "easym68k/sim/effective_addr.h"
#include "easym68k/sim/memory.h"
#include "easym68k/sim/types.h"

namespace easym68k::sim {

// Forward declarations for Trap #15 I/O interfaces (injected via Config)
class ITextIO;
class IFileIO;
class ISerialIO;
class INetworkIO;
class IGraphicsIO;
class ISoundIO;
class IPeripheralIO;
class ISimulatorEnv;
class IPrintIO;
class ILogger;

struct Breakpoint {
  uint32_t address;
  bool enabled;
};

class M68kSimulator {
 public:
  struct Config {
    ITextIO* text_io = nullptr;
    IFileIO* file_io = nullptr;
    ISerialIO* serial_io = nullptr;
    INetworkIO* network_io = nullptr;
    IGraphicsIO* graphics_io = nullptr;
    ISoundIO* sound_io = nullptr;
    IPeripheralIO* peripheral_io = nullptr;
    ISimulatorEnv* sim_env = nullptr;
    IPrintIO* print_io = nullptr;
    ILogger* logger = nullptr;
  };

  explicit M68kSimulator(Config config);
  ~M68kSimulator() = default;

  M68kSimulator(const M68kSimulator&) = delete;
  M68kSimulator& operator=(const M68kSimulator&) = delete;

  // Loads reset vectors from memory and enters supervisor mode.
  void Reset();

  // Executes a single instruction, handles exceptions.
  SimResult Step();

  // Runs until halt, stop_requested_, breakpoint, or max_instructions.
  // max_instructions < 0 means unlimited.
  SimResult Run(int max_instructions = -1);

  // Thread-safe stop request; checked at top of each Run iteration.
  void RequestStop();

  // State and memory accessors
  CpuState& State() { return state_; }
  const CpuState& State() const { return state_; }
  Memory& GetMemory() { return memory_; }
  const Memory& GetMemory() const { return memory_; }

  // Breakpoints
  void AddBreakpoint(uint32_t address);
  void RemoveBreakpoint(uint32_t address);
  void ClearBreakpoints();
  bool HasBreakpoint(uint32_t address) const;

  // Program loading
  bool LoadSRecord(const std::string& filename);
  bool LoadSRecordLines(const std::vector<std::string>& lines);

  // Condition check — public for direct testability.
  bool CheckCondition(Condition cond) const;

 private:
  uint16_t FetchWord();
  uint32_t FetchLong();

  SimResult ExecuteInstruction(uint16_t opcode);

  // EA wrappers that use state_.pc and state_ implicitly.
  EffectiveAddr CalcEA(int mode, int reg, DataSize size);
  uint32_t ReadEA(const EffectiveAddr& ea, DataSize size);
  SimResult WriteEA(const EffectiveAddr& ea, uint32_t value, DataSize size);

  // Pushes stack frame for vector, sets supervisor mode, clears trace, loads new PC.
  void HandleException(ExceptionVector vector);

  // Hierarchical instruction decoder (defined in decode.cc).
  SimResult DispatchGroup0(uint16_t opcode);
  SimResult OpMove(uint16_t opcode);
  SimResult DispatchMoveOrMovea(uint16_t opcode);
  SimResult DispatchGroup4(uint16_t opcode);
  SimResult DispatchGroup5(uint16_t opcode);
  SimResult DispatchGroup6(uint16_t opcode);
  SimResult OpMoveq(uint16_t opcode);
  SimResult DispatchGroup8(uint16_t opcode);
  SimResult DispatchGroup9(uint16_t opcode);
  SimResult OpLine1010(uint16_t opcode);
  SimResult DispatchGroupB(uint16_t opcode);
  SimResult DispatchGroupC(uint16_t opcode);
  SimResult DispatchGroupD(uint16_t opcode);
  SimResult DispatchGroupE(uint16_t opcode);
  SimResult DispatchSimhalt(uint16_t opcode);

  CpuState state_{};
  Memory memory_;
  Config config_;
  std::vector<Breakpoint> breakpoints_;
  std::atomic<bool> stop_requested_{false};
  uint32_t inst_pc_{0};  // PC of the instruction currently executing
  uint32_t start_address_{0};
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_SIMULATOR_H_
