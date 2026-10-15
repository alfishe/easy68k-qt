// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/srecord_loader.h"

#include <cctype>
#include <cstdio>
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
  return {false, 0, false, msg, line, {}, {msg}};
}

std::string LineText(const char* format, int line) {
  char text[64];
  std::snprintf(text, sizeof text, format, line);
  return text;
}

}  // namespace

SRecordLoader::LoadResult SRecordLoader::LoadFromFile(const std::string& filename, Memory& memory) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return MakeError("error: cannot open file " + filename, 0);
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

// SIMOPS2.CPP loadSrec(). Records are read up to the first S7/S8/S9; S5 is
// ignored. A checksum error is reported and loading goes on; a record that
// cannot be read, or data past the 16 MB of memory, stops the load. The
// record length is the byte count (the original counted hex pairs up to the
// end of the line) and the S0 checksum includes its address field.
SRecordLoader::LoadResult SRecordLoader::LoadFromLines(const std::vector<std::string>& lines,
                                                       Memory& memory) {
  LoadResult result{true, 0, false, {}, 0, {}, {}};
  uint8_t* mem = memory.RawData();
  int line_num = 0;
  bool error = false;
  std::string error_text;

  for (const auto& raw : lines) {
    ++line_num;
    std::string line = raw;
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (line.empty())
      continue;  // (the original re-read the previous record type)
    error_text = line;
    error = true;  // until the record is read
    if (line.size() < 4 || line[0] != 'S')
      break;
    const char type = line[1];
    const int byte_count = ParseHexByte(line, 2);
    if (byte_count < 0)
      break;
    int address_bytes = 0;
    switch (type) {
      case '0':
      case '1':
      case '9':
        address_bytes = 2;
        break;
      case '2':
      case '8':
        address_bytes = 3;
        break;
      case '3':
      case '7':
        address_bytes = 4;
        break;
      case '5':
        address_bytes = -1;  // record count: ignored
        break;
      default:
        break;
    }
    if (address_bytes == 0)
      break;
    if (address_bytes < 0) {
      error = false;
      continue;
    }
    if (type >= '7') {  // start address: PC, end of the file (no checksum test)
      bool bad = false;
      for (int i = 0; i < address_bytes; ++i)
        bad = bad || ParseHexByte(line, 4 + static_cast<size_t>(i) * 2) < 0;
      if (bad)
        break;
      result.start_address = ParseHexValue(line, 4, address_bytes) & kAddressMask;
      result.has_start_address = true;
      error = false;
      break;
    }
    if (byte_count < address_bytes + 1 || line.size() < 4 + static_cast<size_t>(byte_count) * 2)
      break;
    std::vector<int> bytes(static_cast<size_t>(byte_count));
    int sum = byte_count;
    bool bad_hex = false;
    for (int i = 0; i < byte_count; ++i) {
      bytes[static_cast<size_t>(i)] = ParseHexByte(line, 4 + static_cast<size_t>(i) * 2);
      if (bytes[static_cast<size_t>(i)] < 0)
        bad_hex = true;
      sum += bytes[static_cast<size_t>(i)];
    }
    if (bad_hex)
      break;
    const bool checksum_ok = (sum & 0xFF) == 0xFF;
    const int data_bytes = byte_count - address_bytes - 1;
    if (type == '0') {
      std::string text = "S0 = ";
      for (int i = 0; i < data_bytes; ++i) {
        const int b = bytes[static_cast<size_t>(address_bytes) + static_cast<size_t>(i)];
        text += b >= ' ' && b <= '~' ? static_cast<char>(b) : '.';
      }
      if (!checksum_ok)
        result.messages.push_back(LineText("Checksum error on line %d...", line_num));
      result.messages.push_back(text);
      result.s0_texts.push_back(std::move(text));
      error = false;
      continue;
    }
    uint32_t address = 0;
    for (int i = 0; i < address_bytes; ++i)
      address = (address << 8) | static_cast<uint32_t>(bytes[static_cast<size_t>(i)]);
    bool bad_address = false;
    for (int i = 0; i < data_bytes; ++i, ++address) {
      if (address > kAddressMask) {
        result.messages.push_back(LineText("Invalid Address on line %d...", line_num));
        bad_address = true;
        break;
      }
      mem[address] =
          static_cast<uint8_t>(bytes[static_cast<size_t>(address_bytes) + static_cast<size_t>(i)]);
    }
    if (!checksum_ok)
      result.messages.push_back(LineText("Checksum error on line %d...", line_num));
    if (bad_address)
      break;
    error = false;
  }

  if (error) {
    result.error_message = LineText("Invalid data on line %d of .S68 file...", line_num);
    result.error_line = line_num;
    result.messages.push_back(result.error_message);
    result.messages.push_back(std::to_string(line_num) + ": " + error_text);
    result.messages.emplace_back("Remainder of load stopped...");
  } else {
    result.messages.emplace_back(".S68 file read successful");
  }
  return result;
}

}  // namespace easym68k::sim
