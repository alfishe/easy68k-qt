// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Task E1 — Automated tests for easy68k-run (`easy68k-example-runner`) and
// discovery/verification of all `examples/**/example.toml` specs.

#include "example_runner.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace easym68k::tools {
namespace {

namespace fs = std::filesystem;

fs::path SourceRoot() {
#ifdef EASY68K_SOURCE_DIR
  return fs::path(EASY68K_SOURCE_DIR);
#else
  return fs::current_path();
#endif
}

TEST(ExampleRunnerTest, ParsesFullTomlSpecSubset) {
  const char* kToml = R"TOML(
# L2-06 — Stack frames with LINK/UNLK
id = "L2-06"
title = "Stack frames with LINK/UNLK"
track = "core"
level = 2
difficulty = 2
requires = ["L2-04", "L2-05"]
source = "main.x68"
max_instructions = 100_000
simhalt = true
exceptions = true

[expect]
halted = true
result = "kHalted"
output = "5! = 120\r\n"
output_contains = ["120"]
registers = { D0 = 0x78, A0 = 0b1010, SR = 0x2000 }
max_cycles = 2_400
min_cycles = 10

[[expect.memory]]
address = 0x2000
bytes = "00 00 00 78"

[[expect.memory]]
address = 0x2010
text = "OK"

[input]
lines = ["5", "42"]
chars = "y\n"
)TOML";

  ExampleSpec spec;
  std::string err;
  ASSERT_TRUE(ParseExampleSpec(kToml, &spec, &err)) << err;
  EXPECT_EQ(spec.id, "L2-06");
  EXPECT_EQ(spec.title, "Stack frames with LINK/UNLK");
  EXPECT_EQ(spec.track, "core");
  EXPECT_EQ(spec.level, 2);
  EXPECT_EQ(spec.difficulty, 2);
  ASSERT_EQ(spec.requires_ids.size(), 2u);
  EXPECT_EQ(spec.requires_ids[0], "L2-04");
  EXPECT_EQ(spec.requires_ids[1], "L2-05");
  EXPECT_EQ(spec.source, "main.x68");
  EXPECT_EQ(spec.max_instructions, 100000u);
  EXPECT_TRUE(spec.simhalt);
  EXPECT_TRUE(spec.exceptions);

  ASSERT_TRUE(spec.expect_halted.has_value());
  EXPECT_TRUE(*spec.expect_halted);
  ASSERT_TRUE(spec.expect_result.has_value());
  EXPECT_EQ(*spec.expect_result, "kHalted");
  ASSERT_TRUE(spec.expect_output.has_value());
  EXPECT_EQ(*spec.expect_output, "5! = 120\r\n");
  ASSERT_EQ(spec.expect_output_contains.size(), 1u);
  EXPECT_EQ(spec.expect_output_contains[0], "120");
  EXPECT_EQ(spec.expect_registers["D0"], 0x78u);
  EXPECT_EQ(spec.expect_registers["A0"], 0x0Au);
  EXPECT_EQ(spec.expect_registers["SR"], 0x2000u);
  ASSERT_TRUE(spec.expect_max_cycles.has_value());
  EXPECT_EQ(*spec.expect_max_cycles, 2400u);
  ASSERT_TRUE(spec.expect_min_cycles.has_value());
  EXPECT_EQ(*spec.expect_min_cycles, 10u);

  ASSERT_EQ(spec.expect_memory.size(), 2u);
  EXPECT_EQ(spec.expect_memory[0].address, 0x2000u);
  EXPECT_EQ(spec.expect_memory[0].bytes, (std::vector<uint8_t>{0x00, 0x00, 0x00, 0x78}));
  EXPECT_EQ(spec.expect_memory[1].address, 0x2010u);
  EXPECT_EQ(spec.expect_memory[1].bytes, (std::vector<uint8_t>{'O', 'K'}));

  ASSERT_EQ(spec.input_lines.size(), 2u);
  EXPECT_EQ(spec.input_lines[0], "5");
  EXPECT_EQ(spec.input_lines[1], "42");
  EXPECT_EQ(spec.input_chars, "y\n");
}

TEST(ExampleRunnerTest, ParsesInlineMemoryArrayAndTextInputAlias) {
  const char* kToml = R"TOML(
id = "L0-03"
title = "Inline memory"
track = "core"
level = 0
difficulty = 1
source = "main.x68"

[expect]
halted = true
memory = [ { address = 0x2000, bytes = "DE AD BE EF" } ]

[input]
text = ["hello"]
)TOML";

  ExampleSpec spec;
  std::string err;
  ASSERT_TRUE(ParseExampleSpec(kToml, &spec, &err)) << err;
  ASSERT_EQ(spec.expect_memory.size(), 1u);
  EXPECT_EQ(spec.expect_memory[0].address, 0x2000u);
  EXPECT_EQ(spec.expect_memory[0].bytes, (std::vector<uint8_t>{0xDE, 0xAD, 0xBE, 0xEF}));
  ASSERT_EQ(spec.input_lines.size(), 1u);
  EXPECT_EQ(spec.input_lines[0], "hello");
}

TEST(ExampleRunnerTest, TemplateExampleAssemblesRunsAndMeetsSpec) {
  const fs::path template_dir = SourceRoot() / "examples" / "_template";
  const auto outcomes = RunExampleDirectory(template_dir.string());
  ASSERT_EQ(outcomes.size(), 1u);
  EXPECT_TRUE(outcomes[0].second.passed);
  EXPECT_TRUE(outcomes[0].second.assembled);
  EXPECT_TRUE(outcomes[0].second.executed);
  EXPECT_TRUE(outcomes[0].second.halted);
  EXPECT_EQ(outcomes[0].second.final_state.d[0], 42u);
  EXPECT_LE(outcomes[0].second.cycles, 100u);
}

TEST(ExampleRunnerTest, ScriptedInputMemoryExpectationsAndExercisesWork) {
  const fs::path tmp_dir = fs::temp_directory_path() / "easy68k_example_runner_test";
  fs::remove_all(tmp_dir);
  fs::create_directories(tmp_dir / "exercises");
  fs::create_directories(tmp_dir / "solutions");

  {
    std::ofstream f(tmp_dir / "main.x68");
    f << "        ORG     $1000\n"
         "START   MOVEQ   #4,D0           ; read number from keyboard\n"
         "        TRAP    #15\n"
         "        ADD.L   #10,D1\n"
         "        MOVE.L  D1,$2000\n"
         "        MOVEQ   #3,D0           ; display signed number in D1.L\n"
         "        TRAP    #15\n"
         "        MOVEQ   #5,D0           ; read single char into D1.B\n"
         "        TRAP    #15\n"
         "        MOVE.B  D1,$2004\n"
         "        SIMHALT\n"
         "        END     START\n";
  }
  {
    std::ofstream f(tmp_dir / "example.toml");
    f << "id = \"TEST-01\"\n"
         "title = \"Scripted I/O\"\n"
         "track = \"core\"\n"
         "level = 1\n"
         "difficulty = 1\n"
         "source = \"main.x68\"\n"
         "[expect]\n"
         "halted = true\n"
         "output = \"42\"\n"
         "registers = { D1 = 0x51 }\n"
         "[[expect.memory]]\n"
         "address = 0x2000\n"
         "bytes = \"00 00 00 2A 51\"\n"
         "[input]\n"
         "lines = [\"32\"]\n"
         "chars = \"Q\"\n";
  }
  {
    std::ofstream f(tmp_dir / "exercises" / "ex1.toml");
    f << "id = \"TEST-01-EX1\"\n"
         "title = \"Exercise 1\"\n"
         "track = \"core\"\n"
         "level = 1\n"
         "difficulty = 1\n"
         "source = \"ex1.x68\"\n"
         "[expect]\n"
         "halted = true\n"
         "registers = { D0 = 99 }\n";
  }
  {
    std::ofstream f(tmp_dir / "solutions" / "ex1.x68");
    f << "        ORG     $1000\n"
         "START   MOVEQ   #99,D0\n"
         "        SIMHALT\n"
         "        END     START\n";
  }

  const auto results = RunExampleDirectory(tmp_dir.string());
  ASSERT_EQ(results.size(), 2u);
  for (const auto& [label, outcome] : results) {
    EXPECT_TRUE(outcome.passed) << label << ": "
                                << (outcome.failures.empty() ? "" : outcome.failures[0]);
  }

  ExampleSpec spec;
  std::string err;
  ASSERT_TRUE(LoadExampleSpecFile((tmp_dir / "example.toml").string(), &spec, &err));
  const std::string json = FormatOutcomeJson(spec, results[0].second);
  EXPECT_NE(json.find("\"passed\": true"), std::string::npos);
  EXPECT_NE(json.find("\"output\": \"42\""), std::string::npos);

  fs::remove_all(tmp_dir);
}

TEST(ExampleRunnerTest, DetectsMismatchAndInteractiveAndToolchainSkip) {
  const fs::path tmp_dir = fs::temp_directory_path() / "easy68k_example_mismatch_test";
  fs::remove_all(tmp_dir);
  fs::create_directories(tmp_dir);

  {
    std::ofstream f(tmp_dir / "main.x68");
    f << "        ORG     $1000\n"
         "START   MOVEQ   #7,D0\n"
         "        SIMHALT\n"
         "        END     START\n";
  }

  // 1. Mismatch on D0 and max_cycles
  ExampleSpec bad_spec;
  bad_spec.id = "BAD";
  bad_spec.source = "main.x68";
  bad_spec.expect_registers["D0"] = 8;
  bad_spec.expect_max_cycles = 1;
  const ExampleRunOutcome bad_out = RunExampleSpec(bad_spec, tmp_dir.string());
  EXPECT_FALSE(bad_out.passed);
  EXPECT_GE(bad_out.failures.size(), 2u);

  // 2. Interactive mode only assembles
  ExampleSpec inter_spec;
  inter_spec.id = "INTER";
  inter_spec.source = "main.x68";
  inter_spec.interactive = true;
  const ExampleRunOutcome inter_out = RunExampleSpec(inter_spec, tmp_dir.string());
  EXPECT_TRUE(inter_out.passed);
  EXPECT_TRUE(inter_out.assembled);
  EXPECT_FALSE(inter_out.executed);

  // 3. gcc-c toolchain without prebuilt .S68 reports skipped
  ExampleSpec c_spec;
  c_spec.id = "C-01";
  c_spec.source = "main.c";
  c_spec.toolchain = "gcc-c";
  const ExampleRunOutcome c_out = RunExampleSpec(c_spec, tmp_dir.string());
  EXPECT_TRUE(c_out.passed);
  EXPECT_TRUE(c_out.skipped);

  fs::remove_all(tmp_dir);
}

TEST(ExampleRunnerTest, AllExamplesInRepositoryPassTheirSpecs) {
  const fs::path examples_root = SourceRoot() / "examples";
  ASSERT_TRUE(fs::is_directory(examples_root));

  std::vector<fs::path> example_dirs;
  for (const auto& entry : fs::recursive_directory_iterator(examples_root)) {
    if (entry.is_regular_file() && entry.path().filename() == "example.toml") {
      example_dirs.push_back(entry.path().parent_path());
    }
  }
  std::sort(example_dirs.begin(), example_dirs.end());
  ASSERT_FALSE(example_dirs.empty());

  for (const fs::path& dir : example_dirs) {
    const auto outcomes = RunExampleDirectory(dir.string());
    ASSERT_FALSE(outcomes.empty()) << dir.string();
    for (const auto& [label, outcome] : outcomes) {
      std::string failure_summary;
      for (const std::string& f : outcome.failures) {
        if (!failure_summary.empty())
          failure_summary += "; ";
        failure_summary += f;
      }
      EXPECT_TRUE(outcome.passed) << label << ": " << failure_summary;
    }
  }
}

}  // namespace
}  // namespace easym68k::tools
