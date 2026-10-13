// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "easym68k/asm/assembler.h"

namespace easym68k::asm_ {
namespace {

std::string FindGoldenDir() {
  std::filesystem::path p = std::filesystem::current_path();
  for (int i = 0; i < 5; ++i) {
    if (std::filesystem::exists(p / "data" / "golden" / "asm")) {
      return (p / "data" / "golden" / "asm").string();
    }
    p = p.parent_path();
  }
  return "data/golden/asm";
}

std::string ReadFile(const std::string& path) {
  std::ifstream f(path, std::ios::in | std::ios::binary);
  if (!f.is_open()) {
    return "";
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

void WriteFile(const std::string& path, const std::string& content) {
  std::ofstream f(path, std::ios::out | std::ios::binary);
  if (f.is_open()) {
    f << content;
  }
}

class GoldenAssemblyTest : public ::testing::TestWithParam<std::string> {
 protected:
  Assembler assembler_;
};

TEST_P(GoldenAssemblyTest, ByteIdenticalOutput) {
  std::string test_name = GetParam();
  std::string golden_dir = FindGoldenDir();
  std::string x68_path = golden_dir + "/" + test_name + ".x68";
  std::string s68_path = golden_dir + "/" + test_name + ".s68";
  std::string l68_path = golden_dir + "/" + test_name + ".l68";

  std::string source = ReadFile(x68_path);
  ASSERT_FALSE(source.empty()) << "Failed to read golden source file: " << x68_path;

  auto result = assembler_.Assemble(source);
  ASSERT_TRUE(result.success) << "Assembly failed for " << test_name << "\n"
                              << (result.errors.empty() ? "" : result.errors[0].message);

  const std::string& generated_s68 = result.srecord;
  const std::string generated_l68 = assembler_.GetListing();

  const char* gen_env = std::getenv("GENERATE_GOLDEN");
  if (gen_env && std::string(gen_env) == "1") {
    // Generate/Bootstrap mode: write the outputs back to the golden folder
    WriteFile(s68_path, generated_s68);
    WriteFile(l68_path, generated_l68);
    SUCCEED() << "Generated golden outputs for " << test_name;
  } else {
    // Verify mode: check that generated output matches golden files byte-for-byte
    std::string golden_s68 = ReadFile(s68_path);
    std::string golden_l68 = ReadFile(l68_path);

    ASSERT_FALSE(golden_s68.empty())
        << "Golden S-Record file is empty or missing. Run with GENERATE_GOLDEN=1 to bootstrap: "
        << s68_path;
    ASSERT_FALSE(golden_l68.empty())
        << "Golden Listing file is empty or missing. Run with GENERATE_GOLDEN=1 to bootstrap: "
        << l68_path;

    EXPECT_EQ(generated_s68, golden_s68) << "S-Record mismatch for " << test_name;
    EXPECT_EQ(generated_l68, golden_l68) << "Listing mismatch for " << test_name;
  }
}

INSTANTIATE_TEST_SUITE_P(GoldenAssemblySuite, GoldenAssemblyTest,
                         ::testing::Values("01_nop", "02_arithmetic", "03_logic", "04_shifts",
                                           "05_moves", "06_branches", "07_comparisons", "08_movem",
                                           "09_stack", "10_directives_dc", "11_directives_ds",
                                           "12_directives_equ", "13_directives_org",
                                           "14_macro_simple", "15_macro_params", "16_macro_narg",
                                           "17_macro_continuation", "18_conditional_assembly",
                                           "19_structured_if", "20_structured_loop",
                                           "21_memory_map"));

}  // namespace
}  // namespace easym68k::asm_
