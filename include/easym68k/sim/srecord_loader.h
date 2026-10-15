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
  // SIMOPS2.CPP loadSrec(). Loading stops at the first S7/S8/S9 record.
  struct LoadResult {
    // False only when the file cannot be opened (the original's FAILURE); a
    // record that stops the load still leaves the data read before it.
    bool success;
    uint32_t start_address;
    bool has_start_address;
    // "Invalid data on line N of .S68 file..." when a record stopped the
    // load, else empty.
    std::string error_message;
    int error_line;  // 1-based line of error_message; 0 if none
    // One entry per S0 record, as loadSrec() shows it: "S0 = " and the data
    // bytes as characters ('.' for those outside ' '..'~').
    std::vector<std::string> s0_texts;
    // The message pane lines of loadSrec(): S0 texts, "Checksum error on
    // line N...", "Invalid Address on line N...", the stop messages, ".S68
    // file read successful".
    std::vector<std::string> messages;
  };

  LoadResult LoadFromFile(const std::string& filename, Memory& memory);
  LoadResult LoadFromStream(std::istream& stream, Memory& memory);
  LoadResult LoadFromLines(const std::vector<std::string>& lines, Memory& memory);
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_SRECORD_LOADER_H_
