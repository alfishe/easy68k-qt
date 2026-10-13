// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// IExecutionLogger — execution and output logging hooks for the simulator.
// Ported from EASy68K/Sim68K/RUN.CPP exec_inst() logging and logU.cpp.
//
// The original writes, for every traced instruction and BEFORE executing it:
//   [memory dump rows]   (Instruction, Registers and Memory)
//   [register block]     (Instruction and Registers, or with Memory)
//   instruction line     (all modes except Disabled)
// and, separately, every character of Trap #15 text output (Output Log Type
// "Text Only"). The core delivers the raw data through this interface; the
// implementation owns formatting and the file.

#ifndef EASY68K_SIM_IEXECUTION_LOGGER_H_
#define EASY68K_SIM_IEXECUTION_LOGGER_H_

#include <atomic>
#include <cstdint>

#include "easym68k/sim/cpu_state.h"
#include "easym68k/sim/memory.h"

namespace easym68k::sim {

class IExecutionLogger {
 public:
  // Execution Log Type (logU.dfm ELogType radio group).
  enum class Mode {
    kOff,                             // Disabled
    kInstructions,                    // Instruction only
    kInstructionsAndRegisters,        // Instruction and Registers
    kInstructionsRegistersAndMemory,  // Instruction, Registers and Memory
  };

  virtual ~IExecutionLogger() = default;

  // Called once per executed instruction, before it executes (as the original
  // does). pc: address of the instruction; opcode: its first word; state: CPU
  // state before execution (state.pc already points past the opcode word).
  virtual void LogInstruction(uint32_t pc, uint16_t opcode, const CpuState& state) = 0;

  // Called before LogInstruction() in kInstructionsRegistersAndMemory mode
  // with the configured dump range.
  virtual void LogMemoryDump(uint32_t start, uint32_t length, const Memory& memory) = 0;

  // Called for Trap #15 text output while output logging is enabled.
  virtual void LogOutput(const char* text, int length) = 0;

  // Configuration (may be changed at runtime from another thread).
  virtual void SetMode(Mode mode) = 0;
  virtual Mode GetMode() const = 0;
  virtual void SetDumpRange(uint32_t start, uint32_t bytes) = 0;
  virtual uint32_t GetDumpStart() const = 0;
  virtual uint32_t GetDumpBytes() const = 0;
  virtual void SetOutputLogging(bool enabled) = 0;  // Output Log Type: Text Only
  virtual bool IsOutputLogging() const = 0;
};

// Convenience base that stores the configuration thread-safely; subclasses
// only implement the three Log* methods.
class ExecutionLoggerBase : public IExecutionLogger {
 public:
  void SetMode(Mode mode) override { mode_.store(mode, std::memory_order_relaxed); }
  Mode GetMode() const override { return mode_.load(std::memory_order_relaxed); }
  void SetDumpRange(uint32_t start, uint32_t bytes) override {
    dump_start_.store(start, std::memory_order_relaxed);
    dump_bytes_.store(bytes, std::memory_order_relaxed);
  }
  uint32_t GetDumpStart() const override { return dump_start_.load(std::memory_order_relaxed); }
  uint32_t GetDumpBytes() const override { return dump_bytes_.load(std::memory_order_relaxed); }
  void SetOutputLogging(bool enabled) override {
    output_logging_.store(enabled, std::memory_order_relaxed);
  }
  bool IsOutputLogging() const override { return output_logging_.load(std::memory_order_relaxed); }

 private:
  std::atomic<Mode> mode_{Mode::kOff};
  std::atomic<uint32_t> dump_start_{0};
  std::atomic<uint32_t> dump_bytes_{0};
  std::atomic<bool> output_logging_{false};
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_IEXECUTION_LOGGER_H_
