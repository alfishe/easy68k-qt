// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Motorola S-Record file loader. No dependency on Cpu or Simulator.

#ifndef EASY68K_SIM_SRECORD_LOADER_H_
#define EASY68K_SIM_SRECORD_LOADER_H_

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "easym68k/sim/memory.h"

namespace easym68k::sim {

class SRecordLoader {
 public:
  struct LoadResult {
    bool success;
    uint32_t start_address;
    bool has_start_address;
    std::string error_message;
    int error_line;  // 1-based; 0 if not line-specific
  };

  LoadResult LoadFromFile(const std::string& filename, Memory& memory);
  LoadResult LoadFromStream(std::istream& stream, Memory& memory);
  LoadResult LoadFromLines(const std::vector<std::string>& lines, Memory& memory);
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_SRECORD_LOADER_H_
