// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Two-pass assembler for EASy68K 68000 Assembler

#ifndef EASYM68K_ASM_ASSEMBLER_H_
#define EASYM68K_ASM_ASSEMBLER_H_

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "easym68k/asm/code_generator.h"
#include "easym68k/asm/error_codes.h"
#include "easym68k/asm/instruction_table.h"
#include "easym68k/asm/parser.h"
#include "easym68k/asm/symbol_table.h"

namespace easym68k::asm_ {

struct AssemblyError {
  int line_number;
  int column;  // 1-based; 0 = unknown
  std::string message;
  ErrorCode code;  // kGeneric for untyped messages
  ErrorSeverity severity;

  AssemblyError()
      : line_number(0), column(0), code(ErrorCode::kGeneric), severity(ErrorSeverity::kError) {}
  AssemblyError(int ln, std::string msg, ErrorCode c = ErrorCode::kGeneric,
                ErrorSeverity sev = ErrorSeverity::kError)
      : line_number(ln), column(0), message(std::move(msg)), code(c), severity(sev) {}
};

struct AssemblyResult {
  bool success;
  uint32_t org;               // Address of first emitted byte
  std::vector<uint8_t> code;  // Contiguous bytes starting at org
  std::vector<AssemblyError> errors;
  int warning_count;

  AssemblyResult() : success(false), org(0), warning_count(0) {}
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

  // Stop assembly after this many errors (default: 10; 0 = unlimited)
  void SetMaxErrors(int n) { max_errors_ = n; }

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

  // Structured control flow state (reset each pass)
  uint32_t stc_label_i_;                // IF label counter (starts 0x00000000)
  uint32_t stc_label_w_;                // WHILE label counter (starts 0x10000000)
  uint32_t stc_label_f_;                // FOR label counter (starts 0x20000000)
  uint32_t stc_label_r_;                // REPEAT label counter (starts 0x30000000)
  uint32_t stc_label_d_;                // DBLOOP label counter (starts 0x40000000)
  std::vector<uint32_t> stc_stack_;     // label values tagged with type mask
  std::vector<char> db_stack_;          // DBLOOP register-number stack
  std::vector<std::string> for_stack_;  // FOR deferred instruction strings

  static constexpr uint32_t kStcMaskI = 0x00000000u;
  static constexpr uint32_t kStcMaskW = 0x10000000u;
  static constexpr uint32_t kStcMaskF = 0x20000000u;
  static constexpr uint32_t kStcMaskR = 0x30000000u;
  static constexpr uint32_t kStcMaskD = 0x40000000u;
  static constexpr uint32_t kStcTypeMask = 0xF0000000u;

  bool IsStructured(const std::string& opcode) const;
  void HandleStructured(const std::string& raw_line, int line_number);
  void InjectLines(const std::vector<std::string>& generated);

  // Macro processor
  static constexpr int kMacroNestLimit = 256;
  std::map<std::string, std::vector<std::string>> macros_;  // name → body lines
  int macro_label_num_;   // per-invocation unique label counter for \@
  int macro_nest_depth_;  // recursion guard

  void ScanMacros(const std::vector<std::string>& lines);
  bool IsMacro(const std::string& opcode) const;
  // Collect body lines [i+1 .. ENDM) from lines, store under name, advance i to ENDM.
  void CollectMacroBody(const std::string& name, const std::vector<std::string>& lines, int& i,
                        int line_number);
  void InvokeMacro(const std::string& name, const std::string& label,
                   const std::string& raw_operands, SizeSpec size, int line_number);

  // Helpers used by InvokeMacro (implemented in macro_processor.cc)
  static std::string ExtractRawOperands(const std::string& raw_line, const std::string& label,
                                        const std::string& opcode);
  static std::vector<std::string> ParseMacroArgs(const std::string& arg_text, char size_char);
  static std::string SubstituteMacroParams(const std::string& body_line,
                                           const std::vector<std::string>& args, int narg,
                                           int label_num);
  static std::string GetLineOpcode(const std::string& raw);
  static int ExtractIfargN(const std::string& raw);

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

  // Error/warning helpers — all ultimately push to errors_.
  // String form: code=kGeneric, severity=kError.
  void AddError(int line_number, const std::string& message);
  // Code form: message = ErrorCodeMessage(code) [+ ": " + extra].
  void AddError(int line_number, ErrorCode code, const std::string& extra = "");
  void AddWarning(int line_number, ErrorCode code, const std::string& extra = "");
  // Internal: push with explicit code + severity (bypasses limit check for warnings).
  void AddError(int line_number, const std::string& message, ErrorCode code,
                ErrorSeverity severity);

  int max_errors_;  // 0 = unlimited; default = 10
};

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_ASSEMBLER_H_
