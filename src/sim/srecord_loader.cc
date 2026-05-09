// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/srecord_loader.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace easym68k::sim {

namespace {

int HexCharToInt(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return -1;
}

// Returns -1 on bad hex or out-of-bounds.
int ParseHexByte(const std::string& line, size_t pos) {
  if (pos + 1 >= line.length())
    return -1;
  int hi = HexCharToInt(line[pos]);
  int lo = HexCharToInt(line[pos + 1]);
  if (hi < 0 || lo < 0)
    return -1;
  return (hi << 4) | lo;
}

// Parse `bytes` bytes of big-endian hex starting at `pos`. Returns 0 on error.
uint32_t ParseHexValue(const std::string& line, size_t pos, int bytes) {
  uint32_t value = 0;
  for (int i = 0; i < bytes; ++i) {
    int b = ParseHexByte(line, pos + static_cast<size_t>(i) * 2);
    if (b < 0)
      return 0;
    value = (value << 8) | static_cast<uint32_t>(b);
  }
  return value;
}

SRecordLoader::LoadResult MakeError(const std::string& msg, int line) {
  return {false, 0, false, msg, line};
}

}  // namespace

SRecordLoader::LoadResult SRecordLoader::LoadFromFile(const std::string& filename, Memory& memory) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return MakeError("cannot open file: " + filename, 0);
  }
  return LoadFromStream(file, memory);
}

SRecordLoader::LoadResult SRecordLoader::LoadFromStream(std::istream& stream, Memory& memory) {
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(stream, line)) {
    lines.push_back(std::move(line));
  }
  return LoadFromLines(lines, memory);
}

SRecordLoader::LoadResult SRecordLoader::LoadFromLines(const std::vector<std::string>& lines,
                                                       Memory& memory) {
  if (lines.empty()) {
    return MakeError("empty input", 0);
  }

  LoadResult result{true, 0, false, {}, 0};
  int line_num = 0;

  for (const auto& raw : lines) {
    ++line_num;

    if (raw.empty())
      continue;
    // Must begin with 'S' or 's'
    if (raw[0] != 'S' && raw[0] != 's')
      continue;
    if (raw.length() < 4)
      continue;

    char type = static_cast<char>(std::toupper(static_cast<unsigned char>(raw[1])));

    int byte_count = ParseHexByte(raw, 2);
    if (byte_count < 0)
      continue;  // malformed byte-count field — skip

    // Every byte after the type+byte_count field must be present.
    size_t required = 4 + static_cast<size_t>(byte_count) * 2;
    if (raw.length() < required) {
      return MakeError("truncated record", line_num);
    }

    // Helper: sum all bytes in the record body (byte_count through last byte).
    // Valid record: (sum & 0xFF) == 0xFF.
    auto validate_checksum = [&](size_t body_start, int body_bytes) -> bool {
      int sum = byte_count;
      for (int i = 0; i < body_bytes; ++i) {
        int b = ParseHexByte(raw, body_start + static_cast<size_t>(i) * 2);
        if (b < 0)
          return false;
        sum += b;
      }
      return (sum & 0xFF) == 0xFF;
    };

    switch (type) {
      case '0':  // Header record — validate checksum but ignore payload
        // No error on bad header checksum; header is purely informational.
        break;

      case '1': {  // 16-bit address data record
        if (byte_count < 3)
          continue;
        if (!validate_checksum(4, byte_count)) {
          return MakeError("checksum mismatch", line_num);
        }
        uint32_t address = ParseHexValue(raw, 4, 2);
        int data_bytes = byte_count - 3;
        size_t data_pos = 8;
        for (int i = 0; i < data_bytes; ++i) {
          int b = ParseHexByte(raw, data_pos);
          if (b < 0)
            return MakeError("invalid hex in data", line_num);
          memory.WriteByte(address++, static_cast<uint8_t>(b));
          data_pos += 2;
        }
        break;
      }

      case '2': {  // 24-bit address data record
        if (byte_count < 4)
          continue;
        if (!validate_checksum(4, byte_count)) {
          return MakeError("checksum mismatch", line_num);
        }
        uint32_t address = ParseHexValue(raw, 4, 3);
        int data_bytes = byte_count - 4;
        size_t data_pos = 10;
        for (int i = 0; i < data_bytes; ++i) {
          int b = ParseHexByte(raw, data_pos);
          if (b < 0)
            return MakeError("invalid hex in data", line_num);
          memory.WriteByte(address++, static_cast<uint8_t>(b));
          data_pos += 2;
        }
        break;
      }

      case '3': {  // 32-bit address data record
        if (byte_count < 5)
          continue;
        if (!validate_checksum(4, byte_count)) {
          return MakeError("checksum mismatch", line_num);
        }
        uint32_t address = ParseHexValue(raw, 4, 4);
        int data_bytes = byte_count - 5;
        size_t data_pos = 12;
        for (int i = 0; i < data_bytes; ++i) {
          int b = ParseHexByte(raw, data_pos);
          if (b < 0)
            return MakeError("invalid hex in data", line_num);
          memory.WriteByte(address++, static_cast<uint8_t>(b));
          data_pos += 2;
        }
        break;
      }

      case '5':  // Record count (16-bit) — ignore
      case '6':  // Record count (24-bit) — ignore
        break;

      case '7': {  // 32-bit start address (terminator for S3)
        if (byte_count < 5)
          continue;
        if (!validate_checksum(4, byte_count)) {
          return MakeError("checksum mismatch", line_num);
        }
        result.start_address = ParseHexValue(raw, 4, 4) & kAddressMask;
        result.has_start_address = true;
        break;
      }

      case '8': {  // 24-bit start address (terminator for S2)
        if (byte_count < 4)
          continue;
        if (!validate_checksum(4, byte_count)) {
          return MakeError("checksum mismatch", line_num);
        }
        result.start_address = ParseHexValue(raw, 4, 3) & kAddressMask;
        result.has_start_address = true;
        break;
      }

      case '9': {  // 16-bit start address (terminator for S1)
        if (byte_count < 3)
          continue;
        if (!validate_checksum(4, byte_count)) {
          return MakeError("checksum mismatch", line_num);
        }
        result.start_address = ParseHexValue(raw, 4, 2) & kAddressMask;
        result.has_start_address = true;
        break;
      }

      default:
        break;
    }
  }

  return result;
}

}  // namespace easym68k::sim
