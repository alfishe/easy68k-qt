// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Memory subsystem for 68000 simulator.

#ifndef EASY68K_SIM_MEMORY_H_
#define EASY68K_SIM_MEMORY_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "easym68k/sim/types.h"

namespace easym68k::sim {

struct MemoryAccessResult {
  bool success;
  uint32_t value;
  SimResult error;
  std::string error_message;
};

// First address/bus error raised by a Read*/Write* call since ClearFault().
// The simulator turns it into a class 0 exception (RUN.CPP exceptionHandler).
struct MemoryFault {
  SimResult kind = SimResult::kOk;  // kAddressError or kBusError
  uint32_t address = 0;             // address as referenced (before masking)
  bool is_read = false;
  uint32_t pc = 0;  // value of the PC source at the time of the access
};

class Memory {
 public:
  Memory();

  Memory(const Memory&) = delete;
  Memory& operator=(const Memory&) = delete;

  Memory(Memory&&) = default;
  Memory& operator=(Memory&&) = default;

  // ---------------------------------------------------------------------------
  // Read Operations
  // ---------------------------------------------------------------------------

  // Read byte; returns error if address is marked kMemInvalid.
  MemoryAccessResult ReadByte(uint32_t address) const;

  // Read word (must be even-aligned).
  MemoryAccessResult ReadWord(uint32_t address) const;

  // Read long (must be even-aligned).
  MemoryAccessResult ReadLong(uint32_t address) const;

  // Read with DataSize selector.
  MemoryAccessResult Read(uint32_t address, DataSize size) const;

  // ---------------------------------------------------------------------------
  // Write Operations
  // ---------------------------------------------------------------------------

  SimResult WriteByte(uint32_t address, uint8_t value);
  SimResult WriteWord(uint32_t address, uint16_t value);
  SimResult WriteLong(uint32_t address, uint32_t value);
  SimResult Write(uint32_t address, uint32_t value, DataSize size);

  // ---------------------------------------------------------------------------
  // Bulk Operations
  // ---------------------------------------------------------------------------

  void Clear();
  // Erases memory to $FF like Sim68K does on start-up and before loading a
  // program (SIM68Ku.cpp FormCreate/OpenFile), and resets the map flags.
  void Erase();
  void LoadData(uint32_t address, const uint8_t* data, size_t size);
  void LoadData(uint32_t address, const std::vector<uint8_t>& data);

  // ---------------------------------------------------------------------------
  // Memory Mapping
  // ---------------------------------------------------------------------------
  // Flag semantics follow UTILS.CPP memoryMapCheck():
  //   kMemInvalid   - bus error on any access
  //   kMemProtected - bus error unless in supervisor mode (see SetSupervisor)
  //   kMemReadOnly  - bus error on write ("Read" map)
  //   kMemRom       - writes are silently ignored

  void SetFlags(uint32_t start_address, uint32_t end_address, uint8_t flags);
  void AddFlags(uint32_t start_address, uint32_t end_address, uint8_t flags);  // OR in
  uint8_t GetFlags(uint32_t address) const;
  bool IsValidRead(uint32_t address) const;
  bool IsValidWrite(uint32_t address) const;
  bool IsRom(uint32_t address) const;

  // Supervisor state used by kMemProtected checks; maintained by the simulator.
  void SetSupervisor(bool supervisor) { supervisor_ = supervisor; }
  bool IsSupervisor() const { return supervisor_; }

  // Address ranges (inclusive) always accessible regardless of flags: the
  // hardware window I/O (7-segment display, LEDs, switches, push buttons).
  void SetMapExemptions(std::vector<std::pair<uint32_t, uint32_t>> ranges) {
    exemptions_ = std::move(ranges);
  }

  // Fault latch: records the first failing access since ClearFault().
  bool HasFault() const { return fault_.kind != SimResult::kOk; }
  const MemoryFault& GetFault() const { return fault_; }
  void ClearFault() { fault_ = MemoryFault{}; }
  // The PC value recorded with a fault is read from *pc (may be null).
  void SetFaultPcSource(const uint32_t* pc) { fault_pc_source_ = pc; }

  // ---------------------------------------------------------------------------
  // Direct Access (for debugging, display, and I/O buffer writes)
  // ---------------------------------------------------------------------------

  uint8_t* RawData() { return data_.data(); }
  const uint8_t* RawData() const { return data_.data(); }
  uint32_t Size() const { return static_cast<uint32_t>(data_.size()); }

 private:
  std::vector<uint8_t> data_;
  std::vector<uint8_t> flags_;
  bool supervisor_ = false;
  std::vector<std::pair<uint32_t, uint32_t>> exemptions_;
  mutable MemoryFault fault_;
  const uint32_t* fault_pc_source_ = nullptr;

  uint32_t MaskAddress(uint32_t address) const { return address & kAddressMask; }
  bool IsAligned(uint32_t address) const { return (address & 1) == 0; }
  bool IsExempt(uint32_t addr) const;
  // Map check over [addr, addr+bytes); returns kOk, kBusError, or kRomIgnore.
  enum class MapResult { kOk, kBusError, kRomIgnore };
  MapResult CheckRead(uint32_t addr, int bytes) const;
  MapResult CheckWrite(uint32_t addr, int bytes) const;
  void RecordFault(SimResult kind, uint32_t address, bool is_read) const;
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_MEMORY_H_
