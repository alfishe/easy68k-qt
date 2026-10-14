// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Golden simulation traces (Task 10.1). Each program in
// data/golden/sim/programs was assembled and run by the ORIGINAL Sim68K core
// (tools/reference/regenerate_sim_golden.sh); the port must reproduce the
// per-instruction register trace (digest), the final state, the memory hash
// and the Trap #15 output exactly. On a mismatch, compare the two simulators
// instruction by instruction with tools/reference/compare_sim.sh.

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "trace_runner.h"

namespace easym68k::tools {
namespace {

namespace fs = std::filesystem;

fs::path GoldenDir() {
  return fs::path(EASY68K_SOURCE_DIR) / "data" / "golden" / "sim" / "programs";
}

std::string ReadFile(const fs::path& p) {
  std::ifstream f(p, std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

std::string ReadStream(FILE* f) {
  std::string s;
  std::rewind(f);
  char buf[4096];
  size_t n;
  while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
    s.append(buf, n);
  return s;
}

// Same digest as regenerate_sim_golden.sh: FNV-1a 64 over the INIT/STEP
// lines (with their newlines); END/FINAL/HASH lines are kept verbatim.
std::string Summarize(const std::string& trace) {
  uint64_t h = 0xcbf29ce484222325ull;
  std::string tail;
  std::istringstream in(trace);
  std::string line;
  while (std::getline(in, line)) {
    line += '\n';
    if (line.rfind("INIT", 0) == 0 || line.rfind("STEP", 0) == 0) {
      for (unsigned char c : line) {
        h ^= c;
        h *= 0x100000001b3ull;
      }
    } else if (line.rfind("END", 0) == 0 || line.rfind("FINAL", 0) == 0 ||
               line.rfind("HASH", 0) == 0) {
      tail += line;
    }
  }
  char head[40];
  std::snprintf(head, sizeof(head), "TRACE %016llX\n", static_cast<unsigned long long>(h));
  return head + tail;
}

std::string StripCr(std::string s) {
  s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
  return s;
}

std::vector<std::string> Programs() {
  std::vector<std::string> names;
  if (fs::exists(GoldenDir()))
    for (const auto& e : fs::directory_iterator(GoldenDir()))
      if (e.path().extension() == ".S68")
        names.push_back(e.path().stem().string());
  std::sort(names.begin(), names.end());
  return names;
}

class GoldenTraceTest : public ::testing::TestWithParam<std::string> {};

TEST_P(GoldenTraceTest, MatchesOriginal) {
  const std::string name = GetParam();
  const fs::path dir = GoldenDir();
  TraceOptions options;
  options.trace_steps = true;
  options.max_steps = 400000;
  if (fs::exists(dir / (name + ".flags"))) {
    std::istringstream flags(ReadFile(dir / (name + ".flags")));
    std::string f;
    while (flags >> f) {
      if (f == "-e")
        options.exceptions = true;
      else if (f == "-b")
        options.bitfield = true;
    }
  }
  FILE* trace = std::tmpfile();
  FILE* out = std::tmpfile();
  FILE* in = fs::exists(dir / (name + ".in")) ? std::fopen((dir / (name + ".in")).c_str(), "rb")
                                              : std::tmpfile();
  ASSERT_TRUE(trace && out && in);
  ASSERT_TRUE(RunTrace((dir / (name + ".S68")).string(), options, trace, in, out));
  std::fflush(trace);
  std::fflush(out);
  EXPECT_EQ(Summarize(ReadStream(trace)), StripCr(ReadFile(dir / (name + ".expected"))));
  EXPECT_EQ(ReadStream(out), ReadFile(dir / (name + ".out")));
  std::fclose(trace);
  std::fclose(out);
  std::fclose(in);
}

INSTANTIATE_TEST_SUITE_P(Programs, GoldenTraceTest, ::testing::ValuesIn(Programs()),
                         [](const ::testing::TestParamInfo<std::string>& info) {
                           return info.param;
                         });

TEST(GoldenTraceSetTest, HasPrograms) {
  EXPECT_GE(Programs().size(), 20u) << GoldenDir();
}

}  // namespace
}  // namespace easym68k::tools
