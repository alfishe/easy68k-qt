// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// S-Record Generator for EASy68K 68000 Assembler (.S68 output)

#include "easym68k/asm/srecord_generator.h"

#include <cstdio>
#include <sstream>

namespace easym68k::asm_ {

SRecordGenerator::SRecordGenerator() : start_address_(0) {}

SRecordGenerator::~SRecordGenerator() = default;

void SRecordGenerator::Reset() {
  start_address_ = 0;
  map_rom_ = false;
  map_rom_start_ = 0;
  map_rom_end_ = 0;
  map_read_ = false;
  map_read_start_ = 0;
  map_read_end_ = 0;
  map_protected_ = false;
  map_protected_start_ = 0;
  map_protected_end_ = 0;
  map_invalid_ = false;
  map_invalid_start_ = 0;
  map_invalid_end_ = 0;
  records_.clear();
}

void SRecordGenerator::SetMemoryMaps(bool map_rom, uint32_t map_rom_start, uint32_t map_rom_end,
                                     bool map_read, uint32_t map_read_start, uint32_t map_read_end,
                                     bool map_protected, uint32_t map_protected_start,
                                     uint32_t map_protected_end, bool map_invalid,
                                     uint32_t map_invalid_start, uint32_t map_invalid_end) {
  map_rom_ = map_rom;
  map_rom_start_ = map_rom_start;
  map_rom_end_ = map_rom_end;
  map_read_ = map_read;
  map_read_start_ = map_read_start;
  map_read_end_ = map_read_end;
  map_protected_ = map_protected;
  map_protected_start_ = map_protected_start;
  map_protected_end_ = map_protected_end;
  map_invalid_ = map_invalid;
  map_invalid_start_ = map_invalid_start;
  map_invalid_end_ = map_invalid_end;
}

void SRecordGenerator::SetStartAddress(uint32_t addr) {
  start_address_ = addr;
}

void SRecordGenerator::AddByte(uint32_t addr, uint8_t byte) {
  if (records_.empty()) {
    records_.push_back({addr, {byte}});
    return;
  }

  Record& last = records_.back();
  uint32_t expected = last.address + static_cast<uint32_t>(last.data.size());
  if (addr == expected && last.data.size() < 36) {
    last.data.push_back(byte);
  } else {
    records_.push_back({addr, {byte}});
  }
}

std::string SRecordGenerator::Generate() const {
  std::ostringstream out;

  // 1. Write standard S0 header record
  // "68KPROG   20CREATED BY EASY68K"
  // The payload hex "36384B50524F47202020323043524541544544204259204541535936384B"
  // decodes directly to the ASCII string "68KPROG   20CREATED BY EASY68K".
  std::string s0_payload_hex = "36384B50524F47202020323043524541544544204259204541535936384B";
  std::vector<uint8_t> s0_payload;
  s0_payload.reserve(30);
  for (size_t i = 0; i < s0_payload_hex.length(); i += 2) {
    char byte_chars[3] = {s0_payload_hex[i], s0_payload_hex[i + 1], '\0'};
    uint8_t b = static_cast<uint8_t>(std::stoul(byte_chars, nullptr, 16));
    s0_payload.push_back(b);
  }
  out << FormatRecord("S0", 0, s0_payload);

  // 2. Write completed data records
  for (const auto& rec : records_) {
    std::string type;
    if ((rec.address & 0xFFFFu) == rec.address) {
      type = "S1";
    } else if ((rec.address & 0xFFFFFFu) == rec.address) {
      type = "S2";
    } else {
      type = "S3";
    }
    out << FormatRecord(type, rec.address, rec.data);
  }

  // 3. Write memory maps S0 records
  if (map_rom_) {
    out << FormatMapRecord("202020202020524F4D20", map_rom_start_, map_rom_end_);
  }
  if (map_read_) {
    out << FormatMapRecord("20202020205245414420", map_read_start_, map_read_end_);
  }
  if (map_protected_) {
    out << FormatMapRecord("50524F54454354454420", map_protected_start_, map_protected_end_);
  }
  if (map_invalid_) {
    out << FormatMapRecord("2020494E56414C494420", map_invalid_start_, map_invalid_end_);
  }

  // 4. Write S8 termination record (24-bit execution start address)
  out << FormatRecord("S8", start_address_, {});

  return out.str();
}

std::string SRecordGenerator::FormatRecord(const std::string& type, uint32_t address,
                                           const std::vector<uint8_t>& data) {
  int addr_size = 2;
  if (type == "S1" || type == "S9") {
    addr_size = 2;
  } else if (type == "S2" || type == "S8") {
    addr_size = 3;
  } else if (type == "S3" || type == "S7") {
    addr_size = 4;
  } else if (type == "S0") {
    addr_size = 2;
  }

  uint8_t count = static_cast<uint8_t>(addr_size + data.size() + 1);

  uint32_t sum = count;
  for (int i = addr_size - 1; i >= 0; --i) {
    sum += (address >> (i * 8)) & 0xFFu;
  }
  for (uint8_t b : data) {
    sum += b;
  }
  uint8_t checksum = ~static_cast<uint8_t>(sum & 0xFFu);

  std::ostringstream ss;
  ss << type;
  char hex[16];
  std::snprintf(hex, sizeof(hex), "%02X", count);
  ss << hex;

  if (addr_size == 2) {
    std::snprintf(hex, sizeof(hex), "%04X", address & 0xFFFFu);
  } else if (addr_size == 3) {
    std::snprintf(hex, sizeof(hex), "%06X", address & 0xFFFFFFu);
  } else {
    std::snprintf(hex, sizeof(hex), "%08X", address);
  }
  ss << hex;

  for (uint8_t b : data) {
    std::snprintf(hex, sizeof(hex), "%02X", b);
    ss << hex;
  }

  std::snprintf(hex, sizeof(hex), "%02X\n", checksum);
  ss << hex;

  return ss.str();
}

std::string SRecordGenerator::FormatMapRecord(const std::string& name_hex, uint32_t start,
                                              uint32_t end) {
  std::vector<uint8_t> data;
  data.reserve(23);
  for (size_t i = 0; i < 20; i += 2) {
    char byte_chars[3] = {name_hex[i], name_hex[i + 1], '\0'};
    uint8_t b = static_cast<uint8_t>(std::stoul(byte_chars, nullptr, 16));
    data.push_back(b);
  }

  char start_str[16];
  std::snprintf(start_str, sizeof(start_str), "%06X", start & 0xFFFFFFu);
  for (int i = 0; i < 6; ++i) {
    data.push_back(static_cast<uint8_t>(start_str[i]));
  }

  data.push_back(static_cast<uint8_t>(','));

  char end_str[16];
  std::snprintf(end_str, sizeof(end_str), "%06X", end & 0xFFFFFFu);
  for (int i = 0; i < 6; ++i) {
    data.push_back(static_cast<uint8_t>(end_str[i]));
  }

  return FormatRecord("S0", 0, data);
}

}  // namespace easym68k::asm_
