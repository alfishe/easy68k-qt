// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// easy68k-run — headless CLI for assembling, running, and verifying
// EASy68K-Qt examples and standalone 68000 programs (Task E1).
//
// Usage:
//   easy68k-run [--json] [--source FILE] [--input-line LINE]...
//               [--input-chars STR] [--max-instructions N]
//               [--dump ADDR:LEN]... [--no-exceptions] [--no-simhalt]
//               <example-dir | example.toml | program.x68 | program.s68>

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "example_runner.h"

namespace {

namespace fs = std::filesystem;

int Usage() {
  std::fprintf(stderr,
               "usage: easy68k-run [--json] [--source FILE] [--input-line LINE]...\n"
               "                   [--input-chars STR] [--max-instructions N]\n"
               "                   [--dump ADDR:LEN]... [--no-exceptions] "
               "[--no-simhalt]\n"
               "                   <example-dir | example.toml | program.x68 | "
               "program.s68>\n");
  return 2;
}

void PrintHumanReport(const std::string& label, const easym68k::tools::ExampleSpec& spec,
                      const easym68k::tools::ExampleRunOutcome& outcome) {
  if (outcome.skipped) {
    std::printf("[SKIP] %s (%s): %s\n", label.c_str(), spec.id.c_str(),
                outcome.skip_reason.c_str());
    return;
  }
  if (outcome.passed) {
    std::printf("[PASS] %s (%s) — %" PRIu64 " instructions, %" PRIu64 " cycles\n", label.c_str(),
                spec.id.empty() ? label.c_str() : spec.id.c_str(), outcome.instructions_executed,
                outcome.cycles);
  } else {
    std::fprintf(stderr, "[FAIL] %s (%s) — %" PRIu64 " instructions, %" PRIu64 " cycles\n",
                 label.c_str(), spec.id.empty() ? label.c_str() : spec.id.c_str(),
                 outcome.instructions_executed, outcome.cycles);
    for (const std::string& failure : outcome.failures) {
      std::fprintf(stderr, "  - %s\n", failure.c_str());
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  bool json_output = false;
  std::string source_override;
  std::vector<std::string> cli_input_lines;
  std::string cli_input_chars;
  uint64_t cli_max_instructions = 0;
  bool has_max_instructions = false;
  bool disable_exceptions = false;
  bool disable_simhalt = false;
  std::vector<std::pair<uint32_t, uint32_t>> dump_ranges;
  std::string target;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--json") {
      json_output = true;
    } else if (arg == "--source" && i + 1 < argc) {
      source_override = argv[++i];
    } else if (arg == "--input-line" && i + 1 < argc) {
      cli_input_lines.emplace_back(argv[++i]);
    } else if (arg == "--input-chars" && i + 1 < argc) {
      cli_input_chars += argv[++i];
    } else if (arg == "--max-instructions" && i + 1 < argc) {
      cli_max_instructions = std::strtoull(argv[++i], nullptr, 0);
      has_max_instructions = true;
    } else if (arg == "--dump" && i + 1 < argc) {
      const char* spec = argv[++i];
      char* sep = nullptr;
      const unsigned long addr = std::strtoul(spec, &sep, 16);
      if (sep == spec || *sep != ':') {
        return Usage();
      }
      char* end = nullptr;
      const unsigned long len = std::strtoul(sep + 1, &end, 16);
      if (end == sep + 1 || *end != '\0') {
        return Usage();
      }
      dump_ranges.emplace_back(static_cast<uint32_t>(addr), static_cast<uint32_t>(len));
    } else if (arg == "--no-exceptions") {
      disable_exceptions = true;
    } else if (arg == "--no-simhalt") {
      disable_simhalt = true;
    } else if ((!arg.empty() && arg[0] == '-') || !target.empty()) {
      return Usage();
    } else {
      target = arg;
    }
  }

  if (target.empty()) {
    return Usage();
  }

  const fs::path target_path(target);
  std::error_code ec;

  // Case 1: directory containing example.toml (and optional exercises/)
  if (fs::is_directory(target_path, ec)) {
    if (!json_output && source_override.empty() && cli_input_lines.empty() &&
        cli_input_chars.empty() && dump_ranges.empty()) {
      const auto outcomes = easym68k::tools::RunExampleDirectory(target);
      if (outcomes.empty()) {
        std::fprintf(stderr, "no example.toml found in directory: %s\n", target.c_str());
        return 2;
      }
      bool all_ok = true;
      for (const auto& [label, out] : outcomes) {
        easym68k::tools::ExampleSpec s;
        std::string ignore_err;
        easym68k::tools::LoadExampleSpecFile(label, &s, &ignore_err);
        PrintHumanReport(label, s, out);
        if (!out.passed)
          all_ok = false;
      }
      return all_ok ? 0 : 1;
    }
    target = (target_path / "example.toml").string();
  }

  easym68k::tools::ExampleSpec spec;
  fs::path base_dir = fs::path(target).parent_path();

  if (fs::path(target).extension() == ".toml") {
    std::string err;
    if (!easym68k::tools::LoadExampleSpecFile(target, &spec, &err)) {
      std::fprintf(stderr, "%s: %s\n", target.c_str(), err.c_str());
      return 2;
    }
  } else {
    // Standalone .x68 / .s68 file
    spec.id = fs::path(target).stem().string();
    spec.title = fs::path(target).filename().string();
    spec.source = fs::path(target).filename().string();
  }

  if (!cli_input_lines.empty()) {
    spec.input_lines = cli_input_lines;
  }
  if (!cli_input_chars.empty()) {
    spec.input_chars = cli_input_chars;
  }
  if (has_max_instructions) {
    spec.max_instructions = cli_max_instructions;
  }
  if (disable_exceptions) {
    spec.exceptions = false;
  }
  if (disable_simhalt) {
    spec.simhalt = false;
  }
  spec.dump_ranges = dump_ranges;

  const easym68k::tools::ExampleRunOutcome outcome =
      easym68k::tools::RunExampleSpec(spec, base_dir.string(), source_override);

  if (json_output) {
    std::cout << easym68k::tools::FormatOutcomeJson(spec, outcome);
  } else {
    PrintHumanReport(target, spec, outcome);
  }

  return outcome.passed ? 0 : 1;
}
