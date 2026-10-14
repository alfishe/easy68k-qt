// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// EASy68K assembler public API. The assembly itself is done by the port of
// the original Edit68K assembler (src/asm/core); this class provides the
// editor's surroundings: editor settings, include file access and the
// results (object code, S-records, listing, messages).

#ifndef EASYM68K_ASM_ASSEMBLER_H_
#define EASYM68K_ASM_ASSEMBLER_H_

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "easym68k/asm/error_codes.h"

namespace easym68k::asm_ {

struct AssemblyError {
  int line_number;
  int column;  // 1-based; 0 = unknown
  std::string message;
  ErrorCode code;  // kGeneric for untyped messages
  ErrorSeverity severity;
  int status = 0;    // original status code (asm.h), 0 if none
  std::string file;  // include file the message belongs to; empty for the source

  AssemblyError()
      : line_number(0), column(0), code(ErrorCode::kGeneric), severity(ErrorSeverity::kError) {}
  AssemblyError(int ln, std::string msg, ErrorCode c = ErrorCode::kGeneric,
                ErrorSeverity sev = ErrorSeverity::kError)
      : line_number(ln), column(0), message(std::move(msg)), code(c), severity(sev) {}
};

struct AssemblySymbol {
  std::string name;  // local labels appear as <global>:<local>
  int32_t value;
  int flags;  // asm.h symbol flags (BACKREF, REDEFINABLE, REG_LIST_SYM, ...)
};

struct AssemblyResult {
  bool success;
  uint32_t org;               // Address of the lowest emitted byte
  std::vector<uint8_t> code;  // Bytes from org to the highest emitted byte (gaps 0)
  std::vector<AssemblyError> errors;
  int warning_count;
  std::string srecord;     // Generated Motorola S-Record text
  uint32_t start_address;  // END operand (S8 record / listing)
  std::vector<AssemblySymbol> symbols;

  AssemblyResult() : success(false), org(0), warning_count(0), start_address(0) {}
};

// Callback for reading included files: given a filename, returns its contents.
// Returns empty string if the file cannot be found/read.
using FileReader = std::function<std::string(const std::string& filename)>;

// The original's Options dialog and editor settings that affect assembly.
struct AssemblerOptions {
  bool generate_listing = true;  // Generate listing (.L68)
  bool generate_srecord = true;  // Generate S-record (.S68)
  bool bitfield = true;          // Assemble bit field instructions
  bool cre = true;               // Symbol table in listing (OPT CRE)
  bool cex = false;              // Expand constants (OPT CEX)
  bool mex = false;              // Expand macro calls (OPT MEX)
  bool sex = false;              // Expand structured code (OPT SEX)
  bool warnings = true;          // Show warnings (OPT WAR)
  bool fixed_tabs = true;        // Editor tab type: fixed (false: assembly tabs)
  int tab_size = 4;              // Editor fixed tab size
};

class Assembler {
 public:
  Assembler();
  ~Assembler();

  // Inject a file reader for INCLUDE/INCBIN (default: read from filesystem)
  void SetFileReader(FileReader reader) { file_reader_ = std::move(reader); }

  // Add a directory to the include search path
  void AddIncludePath(const std::string& path);

  // The original assembler has no error limit; kept for API compatibility.
  void SetMaxErrors(int n) { max_errors_ = n; }

  void SetOptions(const AssemblerOptions& options) { options_ = options; }
  const AssemblerOptions& options() const { return options_; }

  // Assembles `source` (Windows or Unix line ends). Editor settings lines at
  // the end of the file ("*~Tab type~", "*~Tab size~", font lines, as written
  // by Edit68K) are removed and applied, like the original editor does.
  AssemblyResult Assemble(const std::string& source);

  // Listing configuration and retrieval. `date` is the "Created On:" text;
  // `filename` and `page_length` are accepted for compatibility (the
  // original listing has no page header).
  void ConfigureListing(const std::string& filename, const std::string& date, int page_length = 60,
                        bool cex_flag = false);
  std::string GetListing() const { return listing_; }

 private:
  FileReader file_reader_;
  std::vector<std::string> include_paths_;
  AssemblerOptions options_;
  std::string source_filename_;
  std::string created_on_;
  std::string listing_;
  int max_errors_ = 0;

  bool ReadInclude(const std::string& name, std::string* contents) const;
};

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_ASSEMBLER_H_
