// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/c_api.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "easym68k/sim/types.h"

namespace easym68k::sim {
namespace {

TEST(CApiTest, CreateDestroyAndNullSafety) {
  EXPECT_EQ(easy68k_get_register(nullptr, EASY68K_REG_D0), 0u);
  EXPECT_EQ(easy68k_get_cycles(nullptr), 0u);
  EXPECT_EQ(easy68k_is_halted(nullptr), 0);
  EXPECT_EQ(easy68k_is_stopped(nullptr), 0);
  EXPECT_STREQ(easy68k_get_output(nullptr), "");
  EXPECT_EQ(easy68k_load_srecord_file(nullptr, "nonexistent.S68"), 0);
  EXPECT_EQ(easy68k_load_srecord_data(nullptr, "", 0), 0);
  easy68k_sim_destroy(nullptr);

  Easy68kSim* sim = easy68k_sim_create();
  ASSERT_NE(sim, nullptr);
  easy68k_sim_destroy(sim);
}

TEST(CApiTest, RegisterAndMemoryReadWriteAndStep) {
  Easy68kSim* sim = easy68k_sim_create();
  ASSERT_NE(sim, nullptr);

  easy68k_mem_write_long(sim, 0x0000, 0x00004000u);  // SSP
  easy68k_mem_write_long(sim, 0x0004, 0x00001000u);  // PC
  easy68k_mem_write_word(sim, 0x1000, 0x702Au);      // MOVEQ #42,D0
  easy68k_mem_write_word(sim, 0x1002, 0xFFFFu);      // SIMHALT word 1
  easy68k_mem_write_word(sim, 0x1004, 0xFFFFu);      // SIMHALT word 2
  easy68k_mem_write_byte(sim, 0x2000, 0xABu);

  EXPECT_EQ(easy68k_mem_read_byte(sim, 0x2000), 0xABu);
  EXPECT_EQ(easy68k_mem_read_word(sim, 0x1000), 0x702Au);
  EXPECT_EQ(easy68k_mem_read_long(sim, 0x0004), 0x00001000u);

  EXPECT_EQ(easy68k_sim_reset(sim), static_cast<int>(SimResult::kOk));
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_PC), 0x1000u);

  easy68k_set_register(sim, EASY68K_REG_A0, 0x2000u);
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_A0), 0x2000u);

  EXPECT_EQ(easy68k_sim_step(sim), static_cast<int>(SimResult::kOk));
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_D0), 42u);
  EXPECT_GT(easy68k_get_cycles(sim), 0u);

  EXPECT_EQ(easy68k_sim_run(sim, 100), static_cast<int>(SimResult::kHalted));
  EXPECT_EQ(easy68k_is_halted(sim), 1);

  easy68k_clear_cycles(sim);
  EXPECT_EQ(easy68k_get_cycles(sim), 0u);

  easy68k_sim_destroy(sim);
}

TEST(CApiTest, LoadSRecordDataBreakpointsAndTrap15TextIO) {
  Easy68kSim* sim = easy68k_sim_create();
  ASSERT_NE(sim, nullptr);

  // Program at $1000:
  //   $1000: 7004       MOVEQ #4,D0      (Task 4: read number into D1.L)
  //   $1002: 4E4F       TRAP #15
  //   $1004: 7003       MOVEQ #3,D0      (Task 3: display D1.L)
  //   $1006: 4E4F       TRAP #15
  //   $1008: 7009       MOVEQ #9,D0      (Task 9: terminate)
  //   $100A: 4E4F       TRAP #15
  const char* srec =
      "S007000054455354AC\n"
      "S10F100070044E4F70034E4F70094E4FAA\n"
      "S804001000EB\n";
  ASSERT_EQ(easy68k_load_srecord_data(sim, srec, static_cast<int>(std::strlen(srec))), 1);
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_PC), 0x1000u);

  easy68k_queue_input_line(sim, "12345");
  easy68k_add_breakpoint(sim, 0x1004u);

  EXPECT_EQ(easy68k_sim_run(sim, 100), static_cast<int>(SimResult::kOk));
  EXPECT_EQ(easy68k_is_halted(sim), 0);
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_PC), 0x1004u);
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_D1), 12345u);

  easy68k_remove_breakpoint(sim, 0x1004u);
  EXPECT_EQ(easy68k_sim_run(sim, 100), static_cast<int>(SimResult::kHalted));
  EXPECT_STREQ(easy68k_get_output(sim), "12345");

  easy68k_clear_output(sim);
  EXPECT_STREQ(easy68k_get_output(sim), "");

  easy68k_sim_destroy(sim);
}

TEST(CApiTest, StepOverAndSupervisorRegisterAccess) {
  Easy68kSim* sim = easy68k_sim_create();
  ASSERT_NE(sim, nullptr);

  EXPECT_EQ(easy68k_sim_init_program(sim), static_cast<int>(SimResult::kOk));
  easy68k_set_register(sim, EASY68K_REG_SR, 0x2000u);
  easy68k_set_register(sim, EASY68K_REG_USP, 0x00F00000u);
  easy68k_set_register(sim, EASY68K_REG_SSP, 0x00800000u);
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_SR), 0x2000u);
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_USP), 0x00F00000u);
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_SSP), 0x00800000u);

  // $1000: BSR.S $1006 (6104)
  // $1002: SIMHALT     (FFFF FFFF)
  // $1006: MOVEQ #7,D0 (7007)
  // $1008: RTS         (4E75)
  easy68k_mem_write_word(sim, 0x1000, 0x6104u);
  easy68k_mem_write_word(sim, 0x1002, 0xFFFFu);
  easy68k_mem_write_word(sim, 0x1004, 0xFFFFu);
  easy68k_mem_write_word(sim, 0x1006, 0x7007u);
  easy68k_mem_write_word(sim, 0x1008, 0x4E75u);
  easy68k_set_register(sim, EASY68K_REG_PC, 0x1000u);

  EXPECT_EQ(easy68k_sim_step_over(sim), static_cast<int>(SimResult::kOk));
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_PC), 0x1002u);
  EXPECT_EQ(easy68k_get_register(sim, EASY68K_REG_D0), 7u);

  easy68k_clear_breakpoints(sim);
  easy68k_interrupt(sim, 1);
  easy68k_sim_request_stop(sim);
  easy68k_sim_destroy(sim);
}

}  // namespace
}  // namespace easym68k::sim
