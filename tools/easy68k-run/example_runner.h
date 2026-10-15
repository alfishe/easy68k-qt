// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Task E1 — Headless example runner and TOML spec verifier (doc/examples/
// example-spec.md). Shared by the easy68k-run command-line tool and the
// automated example runner test suite.

#ifndef EASY68K_TOOLS_RUN_EXAMPLE_RUNNER_H_
#define EASY68K_TOOLS_RUN_EXAMPLE_RUNNER_H_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "easym68k/sim/cpu_state.h"
#include "easym68k/sim/types.h"

namespace easym68k::tools {

struct MemoryExpectation {
  uint32_t address = 0;
  std::vector<uint8_t> bytes;
};

struct ExampleSpec {
  // Top-level fields (doc/examples/example-spec.md)
  std::string id;
  std::string title;
  std::string track = "core";
  int level = 0;
  int difficulty = 1;
  std::vector<std::string> requires_ids;
  std::string source;
  std::string toolchain = "asm";  // "asm", "gcc-c", "gcc-cpp"
  uint64_t max_instructions = 1000000;
  bool simhalt = true;
  bool exceptions = true;
  bool bitfield = true;
  bool interactive = false;

  // [expect]
  std::optional<bool> expect_halted;
  std::optional<std::string> expect_result;
  std::optional<std::string> expect_output;
  std::vector<std::string> expect_output_contains;
  std::map<std::string, uint32_t> expect_registers;
  std::optional<uint64_t> expect_max_cycles;
  std::optional<uint64_t> expect_min_cycles;
  std::vector<MemoryExpectation> expect_memory;

  // [input]
  std::vector<std::string> input_lines;
  std::string input_chars;

  // Optional extra memory dumps requested via CLI (--dump ADDR:LEN)
  std::vector<std::pair<uint32_t, uint32_t>> dump_ranges;
};

struct ExampleRunOutcome {
  bool passed = false;
  bool skipped = false;
  std::string skip_reason;
  bool assembled = false;
  bool executed = false;
  sim::SimResult last_result = sim::SimResult::kOk;
  bool halted = false;
  bool stopped = false;
  uint64_t instructions_executed = 0;
  uint64_t cycles = 0;
  sim::CpuState final_state{};
  std::string output;
  std::vector<std::pair<uint32_t, std::vector<uint8_t>>> memory_dumps;
  std::vector<std::string> failures;
};

// Parses the strict TOML subset defined in doc/examples/example-spec.md.
bool ParseExampleSpec(std::string_view toml_text, ExampleSpec* out, std::string* error);

// Reads and parses an example.toml file from disk.
bool LoadExampleSpecFile(const std::string& path, ExampleSpec* out, std::string* error);

// Builds, runs, and checks `spec` resolved against `base_dir`.
// If `source_override` is non-empty, it replaces `spec.source` (used when
// verifying `exercises/<name>.toml` against `solutions/<name>.x68|.c|.cpp`).
ExampleRunOutcome RunExampleSpec(const ExampleSpec& spec, const std::string& base_dir,
                                 const std::string& source_override = "");

// Loads `toml_path`, runs it in its parent directory, and returns the outcome.
ExampleRunOutcome RunExampleFile(const std::string& toml_path,
                                 const std::string& source_override = "");

// Runs `example.toml` in `dir_path` plus any `exercises/<name>.toml` paired
// with `solutions/<name>.{x68,X68,c,cpp}`. Returns (label, outcome) pairs.
std::vector<std::pair<std::string, ExampleRunOutcome>> RunExampleDirectory(
    const std::string& dir_path);

// Formats the run outcome and register/memory/output state as JSON.
std::string FormatOutcomeJson(const ExampleSpec& spec, const ExampleRunOutcome& outcome);

// Converts `SimResult` to its canonical enum name (e.g. "kHalted").
std::string SimResultToString(sim::SimResult result);

}  // namespace easym68k::tools

#endif  // EASY68K_TOOLS_RUN_EXAMPLE_RUNNER_H_
