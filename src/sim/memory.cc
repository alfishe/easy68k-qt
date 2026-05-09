// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/memory.h"

#include <algorithm>
#include <cstring>

namespace easym68k::sim {

Memory::Memory() : data_(kMemorySize, 0), flags_(kMemorySize, kMemNormal) {}

// =============================================================================
// Read Operations
// =============================================================================

MemoryAccessResult Memory::ReadByte(uint32_t address) const {
  uint32_t addr = MaskAddress(address);
  if (flags_[addr] & kMemInvalid) {
    return {false, 0, SimResult::kBusError, "invalid memory read"};
  }
  return {true, data_[addr], SimResult::kOk, {}};
}

MemoryAccessResult Memory::ReadWord(uint32_t address) const {
  if (!IsAligned(address)) {
    return {false, 0, SimResult::kAddressError, "word read at odd address"};
  }
  uint32_t addr = MaskAddress(address);
  if ((flags_[addr] | flags_[addr + 1]) & kMemInvalid) {
    return {false, 0, SimResult::kBusError, "invalid memory read"};
  }
  uint32_t value =
      (static_cast<uint32_t>(data_[addr]) << 8) | static_cast<uint32_t>(data_[addr + 1]);
  return {true, value, SimResult::kOk, {}};
}

MemoryAccessResult Memory::ReadLong(uint32_t address) const {
  if (!IsAligned(address)) {
    return {false, 0, SimResult::kAddressError, "long read at odd address"};
  }
  uint32_t addr = MaskAddress(address);
  if ((flags_[addr] | flags_[addr + 1] | flags_[addr + 2] | flags_[addr + 3]) & kMemInvalid) {
    return {false, 0, SimResult::kBusError, "invalid memory read"};
  }
  uint32_t value =
      (static_cast<uint32_t>(data_[addr]) << 24) | (static_cast<uint32_t>(data_[addr + 1]) << 16) |
      (static_cast<uint32_t>(data_[addr + 2]) << 8) | static_cast<uint32_t>(data_[addr + 3]);
  return {true, value, SimResult::kOk, {}};
}

MemoryAccessResult Memory::Read(uint32_t address, DataSize size) const {
  switch (size) {
    case DataSize::kByte:
      return ReadByte(address);
    case DataSize::kWord:
      return ReadWord(address);
    case DataSize::kLong:
      return ReadLong(address);
  }
  return {false, 0, SimResult::kBadInstruction, "unknown data size"};
}

// =============================================================================
// Write Operations
// =============================================================================

SimResult Memory::WriteByte(uint32_t address, uint8_t value) {
  uint32_t addr = MaskAddress(address);
  if (flags_[addr] & (kMemInvalid | kMemRom | kMemProtected)) {
    return SimResult::kBusError;
  }
  data_[addr] = value;
  return SimResult::kOk;
}

SimResult Memory::WriteWord(uint32_t address, uint16_t value) {
  if (!IsAligned(address)) {
    return SimResult::kAddressError;
  }
  uint32_t addr = MaskAddress(address);
  if ((flags_[addr] | flags_[addr + 1]) & (kMemInvalid | kMemRom | kMemProtected)) {
    return SimResult::kBusError;
  }
  data_[addr] = static_cast<uint8_t>(value >> 8);
  data_[addr + 1] = static_cast<uint8_t>(value & 0xFF);
  return SimResult::kOk;
}

SimResult Memory::WriteLong(uint32_t address, uint32_t value) {
  if (!IsAligned(address)) {
    return SimResult::kAddressError;
  }
  uint32_t addr = MaskAddress(address);
  for (int i = 0; i < 4; ++i) {
    if (flags_[addr + i] & (kMemInvalid | kMemRom | kMemProtected)) {
      return SimResult::kBusError;
    }
  }
  data_[addr] = static_cast<uint8_t>(value >> 24);
  data_[addr + 1] = static_cast<uint8_t>(value >> 16);
  data_[addr + 2] = static_cast<uint8_t>(value >> 8);
  data_[addr + 3] = static_cast<uint8_t>(value & 0xFF);
  return SimResult::kOk;
}

SimResult Memory::Write(uint32_t address, uint32_t value, DataSize size) {
  switch (size) {
    case DataSize::kByte:
      return WriteByte(address, static_cast<uint8_t>(value));
    case DataSize::kWord:
      return WriteWord(address, static_cast<uint16_t>(value));
    case DataSize::kLong:
      return WriteLong(address, value);
  }
  return SimResult::kBadInstruction;
}

// =============================================================================
// Bulk Operations
// =============================================================================

void Memory::Clear() {
  std::memset(data_.data(), 0, data_.size());
  std::memset(flags_.data(), kMemNormal, flags_.size());
}

void Memory::LoadData(uint32_t address, const uint8_t* data, size_t size) {
  uint32_t addr = MaskAddress(address);
  size_t copy_size = std::min(size, static_cast<size_t>(kMemorySize - addr));
  std::memcpy(data_.data() + addr, data, copy_size);
}

void Memory::LoadData(uint32_t address, const std::vector<uint8_t>& data) {
  LoadData(address, data.data(), data.size());
}

// =============================================================================
// Memory Mapping
// =============================================================================

void Memory::SetFlags(uint32_t start_address, uint32_t end_address, uint8_t flags) {
  uint32_t start = MaskAddress(start_address);
  uint32_t end = MaskAddress(end_address);
  if (start > end) {
    std::swap(start, end);
  }
  for (uint32_t addr = start; addr <= end && addr < kMemorySize; ++addr) {
    flags_[addr] = flags;
  }
}

uint8_t Memory::GetFlags(uint32_t address) const {
  return flags_[MaskAddress(address)];
}

bool Memory::IsValidRead(uint32_t address) const {
  return !(flags_[MaskAddress(address)] & kMemInvalid);
}

bool Memory::IsValidWrite(uint32_t address) const {
  uint8_t f = flags_[MaskAddress(address)];
  return !(f & (kMemInvalid | kMemRom | kMemProtected));
}

bool Memory::IsRom(uint32_t address) const {
  return (flags_[MaskAddress(address)] & kMemRom) != 0;
}

}  // namespace easym68k::sim
