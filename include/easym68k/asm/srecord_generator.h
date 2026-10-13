// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// S-Record Generator for EASy68K 68000 Assembler (.S68 output)

#ifndef EASYM68K_ASM_SRECORD_GENERATOR_H_
#define EASYM68K_ASM_SRECORD_GENERATOR_H_

#include <cstdint>
#include <string>
#include <vector>

namespace easym68k::asm_ {

class SRecordGenerator {
 public:
  SRecordGenerator();
  ~SRecordGenerator();

  // Reset generator state
  void Reset();

  // Configure memory map options
  void SetMemoryMaps(bool map_rom, uint32_t map_rom_start, uint32_t map_rom_end, bool map_read,
                     uint32_t map_read_start, uint32_t map_read_end, bool map_protected,
                     uint32_t map_protected_start, uint32_t map_protected_end, bool map_invalid,
                     uint32_t map_invalid_start, uint32_t map_invalid_end);

  // Set the starting execution address
  void SetStartAddress(uint32_t addr);

  // Add a byte of machine code at a specific address
  void AddByte(uint32_t addr, uint8_t byte);

  // Add an indivisible item (1, 2 or 4 bytes, big-endian order) at addr,
  // following the original's record-splitting rule.
  void AddItem(uint32_t addr, const uint8_t* bytes, int size);

  // Generate complete S-Record output text
  std::string Generate() const;

 private:
  // Format a single record dynamically
  static std::string FormatRecord(const std::string& type, uint32_t address,
                                  const std::vector<uint8_t>& data);

  // Format S0 memory map record
  static std::string FormatMapRecord(const std::string& name_hex, uint32_t start, uint32_t end);

  uint32_t start_address_;

  // Memory map configuration
  bool map_rom_ = false;
  uint32_t map_rom_start_ = 0;
  uint32_t map_rom_end_ = 0;

  bool map_read_ = false;
  uint32_t map_read_start_ = 0;
  uint32_t map_read_end_ = 0;

  bool map_protected_ = false;
  uint32_t map_protected_start_ = 0;
  uint32_t map_protected_end_ = 0;

  bool map_invalid_ = false;
  uint32_t map_invalid_start_ = 0;
  uint32_t map_invalid_end_ = 0;

  struct Record {
    uint32_t address;
    std::vector<uint8_t> data;
  };

  mutable std::vector<Record> records_;
};

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_SRECORD_GENERATOR_H_
