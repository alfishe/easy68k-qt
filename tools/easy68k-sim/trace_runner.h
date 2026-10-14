// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Headless program runner with register tracing, shared by the easy68k-sim
// command-line tool and the golden simulation trace test (Task 10.1). The
// trace format matches tools/reference/sim68k (easy68k-ref-sim), the oracle
// built from the original Sim68K sources.

#ifndef EASY68K_TOOLS_SIM_TRACE_RUNNER_H_
#define EASY68K_TOOLS_SIM_TRACE_RUNNER_H_

#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace easym68k::tools {

struct TraceOptions {
  bool exceptions = false;  // Sim68K /e (off by default, as in Sim68K)
  bool bitfield = false;    // Sim68K /b
  long max_steps = 100000;
  bool trace_steps = false;                          // one line per instruction
  std::vector<std::pair<uint32_t, uint32_t>> dumps;  // memory dumps (addr, len)
};

// Loads `s68_path` the way Sim68K's File > Open does and runs it until it
// halts, stops or reaches max_steps. Trace lines (INIT/STEP/END/FINAL/HASH/
// MEM) go to `trace`; Trap #15 text output to `out`; keyboard input is read
// from `in`. Returns false if the file could not be loaded.
bool RunTrace(const std::string& s68_path, const TraceOptions& options, FILE* trace, FILE* in,
              FILE* out);

}  // namespace easym68k::tools

#endif  // EASY68K_TOOLS_SIM_TRACE_RUNNER_H_
