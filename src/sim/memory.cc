// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/memory.h"

#include <algorithm>
#include <cstring>

namespace easym68k::sim {

Memory::Memory() : data_(kMemorySize, 0), flags_(kMemorySize, kMemNormal) {}

// =============================================================================
// Memory map checks (UTILS.CPP memoryMapCheck() / mem_req())
// =============================================================================

bool Memory::IsExempt(uint32_t addr) const {
  for (const auto& r : exemptions_) {
    if (addr >= r.first && addr <= r.second)
      return true;
  }
  return false;
}

// mem_req() checks the Invalid and Protected maps at the first byte only.
Memory::MapResult Memory::CheckRead(uint32_t addr, int /*bytes*/) const {
  uint8_t f = flags_[addr];
  if (f == kMemNormal || IsExempt(addr))
    return MapResult::kOk;
  if (f & kMemInvalid)
    return MapResult::kBusError;
  if ((f & kMemProtected) && !supervisor_)
    return MapResult::kBusError;
  return MapResult::kOk;
}

// memoryMapCheck(Invalid | Protected | Read | Rom) tests the whole access range;
// ROM is checked last and makes the write a silent no-op.
Memory::MapResult Memory::CheckWrite(uint32_t addr, int bytes) const {
  if (IsExempt(addr))
    return MapResult::kOk;
  uint8_t f = 0;
  for (int i = 0; i < bytes; ++i)
    f |= flags_[(addr + i) & kAddressMask];
  if (f == kMemNormal)
    return MapResult::kOk;
  if (f & kMemInvalid)
    return MapResult::kBusError;
  if ((f & kMemProtected) && !supervisor_)
    return MapResult::kBusError;
  if (f & kMemReadOnly)
    return MapResult::kBusError;
  if (f & kMemRom)
    return MapResult::kRomIgnore;
  return MapResult::kOk;
}

void Memory::RecordFault(SimResult kind, uint32_t address, bool is_read) const {
  if (fault_.kind != SimResult::kOk)
    return;  // keep the first fault
  fault_.kind = kind;
  fault_.address = address;
  fault_.is_read = is_read;
  fault_.pc = fault_pc_source_ ? *fault_pc_source_ : 0;
}

// =============================================================================
// Read Operations
// =============================================================================

MemoryAccessResult Memory::ReadByte(uint32_t address) const {
  uint32_t addr = MaskAddress(address);
  if (CheckRead(addr, 1) == MapResult::kBusError) {
    RecordFault(SimResult::kBusError, address, true);
    return {false, 0, SimResult::kBusError, "bus error on read"};
  }
  NoteRead(addr);
  return {true, data_[addr], SimResult::kOk, {}};
}

MemoryAccessResult Memory::ReadWord(uint32_t address) const {
  if (!IsAligned(address)) {
    RecordFault(SimResult::kAddressError, address, true);
    return {false, 0, SimResult::kAddressError, "word read at odd address"};
  }
  uint32_t addr = MaskAddress(address);
  if (CheckRead(addr, 2) == MapResult::kBusError) {
    RecordFault(SimResult::kBusError, address, true);
    return {false, 0, SimResult::kBusError, "bus error on read"};
  }
  uint32_t value = (static_cast<uint32_t>(data_[addr]) << 8) |
                   static_cast<uint32_t>(data_[(addr + 1) & kAddressMask]);
  NoteRead(addr);
  return {true, value, SimResult::kOk, {}};
}

MemoryAccessResult Memory::ReadLong(uint32_t address) const {
  if (!IsAligned(address)) {
    RecordFault(SimResult::kAddressError, address, true);
    return {false, 0, SimResult::kAddressError, "long read at odd address"};
  }
  uint32_t addr = MaskAddress(address);
  if (CheckRead(addr, 4) == MapResult::kBusError) {
    RecordFault(SimResult::kBusError, address, true);
    return {false, 0, SimResult::kBusError, "bus error on read"};
  }
  uint32_t value = (static_cast<uint32_t>(data_[addr]) << 24) |
                   (static_cast<uint32_t>(data_[(addr + 1) & kAddressMask]) << 16) |
                   (static_cast<uint32_t>(data_[(addr + 2) & kAddressMask]) << 8) |
                   static_cast<uint32_t>(data_[(addr + 3) & kAddressMask]);
  NoteRead(addr);
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
  if (fault_blocks_writes_ && fault_.kind != SimResult::kOk)
    return fault_.kind;
  uint32_t addr = MaskAddress(address);
  switch (CheckWrite(addr, 1)) {
    case MapResult::kBusError:
      RecordFault(SimResult::kBusError, address, false);
      return SimResult::kBusError;
    case MapResult::kRomIgnore:
      return SimResult::kOk;
    case MapResult::kOk:
      break;
  }
  data_[addr] = value;
  NoteWrite(addr);
  return SimResult::kOk;
}

SimResult Memory::WriteWord(uint32_t address, uint16_t value) {
  if (fault_blocks_writes_ && fault_.kind != SimResult::kOk)
    return fault_.kind;
  if (!IsAligned(address)) {
    RecordFault(SimResult::kAddressError, address, false);
    return SimResult::kAddressError;
  }
  uint32_t addr = MaskAddress(address);
  switch (CheckWrite(addr, 2)) {
    case MapResult::kBusError:
      RecordFault(SimResult::kBusError, address, false);
      return SimResult::kBusError;
    case MapResult::kRomIgnore:
      return SimResult::kOk;
    case MapResult::kOk:
      break;
  }
  data_[addr] = static_cast<uint8_t>(value >> 8);
  data_[(addr + 1) & kAddressMask] = static_cast<uint8_t>(value & 0xFF);
  NoteWrite(addr);
  return SimResult::kOk;
}

SimResult Memory::WriteLong(uint32_t address, uint32_t value) {
  if (fault_blocks_writes_ && fault_.kind != SimResult::kOk)
    return fault_.kind;
  if (!IsAligned(address)) {
    RecordFault(SimResult::kAddressError, address, false);
    return SimResult::kAddressError;
  }
  uint32_t addr = MaskAddress(address);
  switch (CheckWrite(addr, 4)) {
    case MapResult::kBusError:
      RecordFault(SimResult::kBusError, address, false);
      return SimResult::kBusError;
    case MapResult::kRomIgnore:
      return SimResult::kOk;
    case MapResult::kOk:
      break;
  }
  data_[addr] = static_cast<uint8_t>(value >> 24);
  data_[(addr + 1) & kAddressMask] = static_cast<uint8_t>(value >> 16);
  data_[(addr + 2) & kAddressMask] = static_cast<uint8_t>(value >> 8);
  data_[(addr + 3) & kAddressMask] = static_cast<uint8_t>(value & 0xFF);
  NoteWrite(addr);
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
  fault_ = MemoryFault{};
  supervisor_ = false;
}

void Memory::Erase() {
  std::memset(data_.data(), 0xFF, data_.size());
  std::memset(flags_.data(), kMemNormal, flags_.size());
  fault_ = MemoryFault{};
  supervisor_ = false;
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

void Memory::AddFlags(uint32_t start_address, uint32_t end_address, uint8_t flags) {
  uint32_t start = MaskAddress(start_address);
  uint32_t end = MaskAddress(end_address);
  if (start > end) {
    std::swap(start, end);
  }
  for (uint32_t addr = start; addr <= end && addr < kMemorySize; ++addr) {
    flags_[addr] |= flags;
  }
}

uint8_t Memory::GetFlags(uint32_t address) const {
  return flags_[MaskAddress(address)];
}

bool Memory::IsValidRead(uint32_t address) const {
  return CheckRead(MaskAddress(address), 1) != MapResult::kBusError;
}

bool Memory::IsValidWrite(uint32_t address) const {
  return CheckWrite(MaskAddress(address), 1) != MapResult::kBusError;
}

bool Memory::IsRom(uint32_t address) const {
  return (flags_[MaskAddress(address)] & kMemRom) != 0;
}

}  // namespace easym68k::sim
