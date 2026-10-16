// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// C API wrapper for the headless M68000 simulator (doc/future_backlog.md #6).
// Pure C99/C++17 header with zero Qt dependencies.

#ifndef EASY68K_SIM_C_API_H_
#define EASY68K_SIM_C_API_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Easy68kSim Easy68kSim;

// Register indices for easy68k_get_register / easy68k_set_register.
enum Easy68kReg {
  EASY68K_REG_D0 = 0,
  EASY68K_REG_D1 = 1,
  EASY68K_REG_D2 = 2,
  EASY68K_REG_D3 = 3,
  EASY68K_REG_D4 = 4,
  EASY68K_REG_D5 = 5,
  EASY68K_REG_D6 = 6,
  EASY68K_REG_D7 = 7,
  EASY68K_REG_A0 = 8,
  EASY68K_REG_A1 = 9,
  EASY68K_REG_A2 = 10,
  EASY68K_REG_A3 = 11,
  EASY68K_REG_A4 = 12,
  EASY68K_REG_A5 = 13,
  EASY68K_REG_A6 = 14,
  EASY68K_REG_A7 = 15,
  EASY68K_REG_PC = 16,
  EASY68K_REG_SR = 17,
  EASY68K_REG_USP = 18,
  EASY68K_REG_SSP = 19,
};

// Lifecycle
Easy68kSim* easy68k_sim_create(void);
void easy68k_sim_destroy(Easy68kSim* sim);

// Execution (returns SimResult integer code; 0 = kOk, 1 = kHalted, etc.)
int easy68k_sim_reset(Easy68kSim* sim);
int easy68k_sim_init_program(Easy68kSim* sim);
int easy68k_sim_step(Easy68kSim* sim);
int easy68k_sim_step_over(Easy68kSim* sim);
int easy68k_sim_run(Easy68kSim* sim, int max_instructions);
void easy68k_sim_request_stop(Easy68kSim* sim);

// State access
uint32_t easy68k_get_register(const Easy68kSim* sim, int reg);
void easy68k_set_register(Easy68kSim* sim, int reg, uint32_t value);
uint64_t easy68k_get_cycles(const Easy68kSim* sim);
void easy68k_clear_cycles(Easy68kSim* sim);
int easy68k_is_halted(const Easy68kSim* sim);
int easy68k_is_stopped(const Easy68kSim* sim);

// Memory access
uint8_t easy68k_mem_read_byte(const Easy68kSim* sim, uint32_t addr);
uint16_t easy68k_mem_read_word(const Easy68kSim* sim, uint32_t addr);
uint32_t easy68k_mem_read_long(const Easy68kSim* sim, uint32_t addr);
void easy68k_mem_write_byte(Easy68kSim* sim, uint32_t addr, uint8_t val);
void easy68k_mem_write_word(Easy68kSim* sim, uint32_t addr, uint16_t val);
void easy68k_mem_write_long(Easy68kSim* sim, uint32_t addr, uint32_t val);

// Loading (returns 1 on success, 0 on failure)
int easy68k_load_srecord_file(Easy68kSim* sim, const char* path);
int easy68k_load_srecord_data(Easy68kSim* sim, const char* data, int len);

// Breakpoints
void easy68k_add_breakpoint(Easy68kSim* sim, uint32_t addr);
void easy68k_remove_breakpoint(Easy68kSim* sim, uint32_t addr);
void easy68k_clear_breakpoints(Easy68kSim* sim);

// Interrupts
void easy68k_interrupt(Easy68kSim* sim, int level);

// Built-in buffered Trap #15 text I/O & custom interface injection
const char* easy68k_get_output(const Easy68kSim* sim);
void easy68k_clear_output(Easy68kSim* sim);
void easy68k_queue_input_line(Easy68kSim* sim, const char* line);
void easy68k_set_text_io(Easy68kSim* sim, void* text_io);
void easy68k_set_file_io(Easy68kSim* sim, void* file_io);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // EASY68K_SIM_C_API_H_
