// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Two-pass assembler for EASy68K 68000 Assembler

#ifndef EASYM68K_ASM_ASSEMBLER_H_
#define EASYM68K_ASM_ASSEMBLER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "easym68k/asm/parser.h"
#include "easym68k/asm/symbol_table.h"

namespace easym68k::asm_ {

struct AssemblyError {
  int line_number;
  std::string message;
};

struct AssemblyResult {
  bool success;
  uint32_t org;               // Address of first emitted byte
  std::vector<uint8_t> code;  // Contiguous bytes starting at org
  std::vector<AssemblyError> errors;

  AssemblyResult() : success(false), org(0) {}
};

class Assembler {
 public:
  Assembler();
  ~Assembler();

  AssemblyResult Assemble(const std::string& source);

 private:
  int pass_;
  uint32_t location_counter_;
  uint32_t org_;
  bool org_set_;

  std::vector<uint8_t> code_;
  std::vector<AssemblyError> errors_;

  Parser parser_;
  SymbolTable symbols_;

  static std::vector<std::string> SplitLines(const std::string& source);

  void RunPass(const std::vector<std::string>& lines);
  bool ProcessLine(const ParsedLine& line);
  bool HandleDirective(const ParsedLine& line);
  bool HandleInstruction(const ParsedLine& line);
  int InstructionSize(const ParsedLine& line) const;

  void EmitByte(uint8_t b);
  void EmitWord(uint16_t w);
  void EmitLong(uint32_t l);
  void AddError(int line_number, const std::string& message);
};

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_ASSEMBLER_H_
