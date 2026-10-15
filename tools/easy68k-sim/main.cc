// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// easy68k-sim: headless command-line runner for the EASy68K-Qt simulator core.
//
//   easy68k-sim [-e] [-b] [-n MAX] [-t] [-o TRACE] [-m ADDR:LEN]... [-d DIR] FILE.S68
//
//   -e   exceptions enabled (Sim68K /e; off by default as in Sim68K)
//   -b   bitfield instructions enabled (Sim68K /b)
//   -n   stop after MAX instructions (default 100000)
//   -t   write one register line per executed instruction
//   -o   trace file (default stderr)
//   -m   dump LEN bytes of memory at ADDR (hex) after the run
//   -d   directory for Trap #15 file tasks (default: current directory)
//
// Loads the program the way Sim68K's File > Open does (memory erased to $FF,
// toggle-switch byte cleared, registers initialized) and runs it. Trap #15
// text output goes to stdout and text input comes from stdin. The trace
// format matches tools/reference/sim68k (easy68k-ref-sim), the oracle built
// from the original sources, so tools/reference/compare_sim.sh can diff them.

#include <cstdio>
#include <cstdlib>
#include <string>

#include "trace_runner.h"

namespace {

int Usage() {
  std::fprintf(
      stderr,
      "usage: easy68k-sim [-e] [-b] [-n MAX] [-t] [-o TRACE] [-m ADDR:LEN]... [-d DIR] FILE.S68\n");
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  easym68k::tools::TraceOptions options;
  FILE* trace = stderr;
  const char* file = nullptr;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "-e") {
      options.exceptions = true;
    } else if (a == "-b") {
      options.bitfield = true;
    } else if (a == "-t") {
      options.trace_steps = true;
    } else if (a == "-n" && i + 1 < argc) {
      options.max_steps = std::strtol(argv[++i], nullptr, 0);
    } else if (a == "-o" && i + 1 < argc) {
      trace = std::fopen(argv[++i], "w");
      if (!trace) {
        std::perror(argv[i]);
        return 2;
      }
    } else if (a == "-d" && i + 1 < argc) {
      options.file_dir = argv[++i];
    } else if (a == "-m" && i + 1 < argc) {
      unsigned addr = 0, len = 0;
      if (std::sscanf(argv[++i], "%x:%x", &addr, &len) != 2)
        return Usage();
      options.dumps.emplace_back(addr, len);
    } else if (!a.empty() && a[0] != '-' && !file) {
      file = argv[i];
    } else {
      return Usage();
    }
  }
  if (!file)
    return Usage();
  bool ok = easym68k::tools::RunTrace(file, options, trace, stdin, stdout);
  if (trace != stderr)
    std::fclose(trace);
  return ok ? 0 : 1;
}
