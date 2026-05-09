// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Memory subsystem for 68000 simulator.

#ifndef EASY68K_SIM_MEMORY_H_
#define EASY68K_SIM_MEMORY_H_

#include <cstdint>
#include <string>
#include <vector>

#include "easym68k/sim/types.h"

namespace easym68k::sim {

struct MemoryAccessResult {
  bool success;
  uint32_t value;
  SimResult error;
  std::string error_message;
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
  void LoadData(uint32_t address, const uint8_t* data, size_t size);
  void LoadData(uint32_t address, const std::vector<uint8_t>& data);

  // ---------------------------------------------------------------------------
  // Memory Mapping
  // ---------------------------------------------------------------------------

  void SetFlags(uint32_t start_address, uint32_t end_address, uint8_t flags);
  uint8_t GetFlags(uint32_t address) const;
  bool IsValidRead(uint32_t address) const;
  bool IsValidWrite(uint32_t address) const;
  bool IsRom(uint32_t address) const;

  // ---------------------------------------------------------------------------
  // Direct Access (for debugging/display)
  // ---------------------------------------------------------------------------

  const uint8_t* RawData() const { return data_.data(); }
  uint32_t Size() const { return static_cast<uint32_t>(data_.size()); }

 private:
  std::vector<uint8_t> data_;
  std::vector<uint8_t> flags_;

  uint32_t MaskAddress(uint32_t address) const { return address & kAddressMask; }
  bool IsAligned(uint32_t address) const { return (address & 1) == 0; }
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_MEMORY_H_
