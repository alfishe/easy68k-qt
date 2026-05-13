// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Two-pass assembler for EASy68K 68000 Assembler

#ifndef EASYM68K_ASM_ASSEMBLER_H_
#define EASYM68K_ASM_ASSEMBLER_H_

#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include "easym68k/asm/code_generator.h"
#include "easym68k/asm/instruction_table.h"
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

// Callback for reading included files: given a filename, returns its contents.
// Returns empty string if the file cannot be found/read.
using FileReader = std::function<std::string(const std::string& filename)>;

class Assembler {
 public:
  Assembler();
  ~Assembler();

  // Inject a file reader for INCLUDE/INCBIN (default: read from filesystem)
  void SetFileReader(FileReader reader) { file_reader_ = std::move(reader); }

  // Add a directory to the include search path
  void AddIncludePath(const std::string& path);

  AssemblyResult Assemble(const std::string& source);

 private:
  int pass_;
  uint32_t location_counter_;
  uint32_t org_;
  bool org_set_;
  bool end_seen_;
  bool listing_enabled_;

  // OFFSET mode: saves location counter; exits on ORG/SECTION
  bool offset_mode_;
  uint32_t offset_save_loc_;

  // SECTION support: 16 sections, each with a saved location counter
  static constexpr int kNumSections = 16;
  uint32_t section_locs_[kNumSections];
  int current_section_;

  std::vector<uint8_t> code_;
  std::vector<AssemblyError> errors_;
  std::vector<std::string> include_paths_;

  // Include cycle detection: set of filenames currently being assembled
  std::set<std::string> active_includes_;

  // Conditional assembly stack (IFC/IFNC/IFxx/ELSE/ENDC)
  struct CondFrame {
    bool active;     // true = current branch is being assembled
    bool seen_else;  // true = ELSE already encountered for this frame
  };
  std::vector<CondFrame> cond_stack_;

  bool IsAssembling() const;       // true when no enclosing conditional is inactive
  bool OuterIsAssembling() const;  // true when all frames *except* the top are active

  FileReader file_reader_;

  Parser parser_;
  SymbolTable symbols_;
  InstructionTable instruction_table_;
  CodeGenerator code_generator_;

  static std::vector<std::string> SplitLines(const std::string& source);

  // Reads a file via file_reader_ (with include path search for INCLUDE)
  std::string ReadIncludeFile(const std::string& filename);
  // Reads a binary file for INCBIN (raw bytes)
  std::string ReadBinaryFile(const std::string& filename);

  void RunPass(const std::vector<std::string>& lines);
  bool ProcessLine(const ParsedLine& line);
  bool HandleDirective(const ParsedLine& line);
  bool HandleInstruction(const ParsedLine& line);
  int InstructionSize(const ParsedLine& line) const;

  // Conditional assembly — always called regardless of IsAssembling()
  bool IsConditional(const std::string& opcode) const;
  void HandleConditional(const ParsedLine& line);

  void EmitByte(uint8_t b);
  void EmitWord(uint16_t w);
  void EmitLong(uint32_t l);
  void WordAlign();
  void AddError(int line_number, const std::string& message);
};

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_ASSEMBLER_H_
