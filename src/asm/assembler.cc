// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Assembler API around the port of the original Edit68K assembler.

#include "easym68k/asm/assembler.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/asm_core.h"
#include "easym68k/asm/original_status.h"

namespace easym68k::asm_ {

namespace {

// Port error code for an original status (for callers that classify
// diagnostics); the message text is always the original's.
ErrorCode CodeForStatus(int st) {
  switch (st) {
    case status::kInvAddrMode:
      return ErrorCode::kInvEa;
    case status::kInvSizeCode:
      return ErrorCode::kInvSizeCode;
    case status::kInvQuickConst:
      return ErrorCode::kInvQuickConst;
    case status::kInvMoveQuickConst:
      return ErrorCode::kInvMoveQuickConst;
    case status::kInvVectorNum:
      return ErrorCode::kInvVectorNum;
    case status::kInvBranchDisp:
      return ErrorCode::kInvBranchDisp;
    case status::kInvDisp:
      return ErrorCode::kInvDisp;
    case status::kInvAbsAddress:
      return ErrorCode::kInvAbsAddress;
    case status::kInv8BitData:
      return ErrorCode::kInv8BitData;
    case status::kInv16BitData:
      return ErrorCode::kInv16BitData;
    case status::kInvShiftCount:
      return ErrorCode::kInvShiftCount;
    case status::kInvOpcode:
      return ErrorCode::kUnknownInstruction;
    case status::kUndefined:
      return ErrorCode::kUndefSymbol;
    case status::kInvForwardRef:
      return ErrorCode::kInvForwardRef;
    case status::kMultipleDefs:
      return ErrorCode::kMultDefd;
    case status::kDivByZero:
      return ErrorCode::kDivByZero;
    case status::kOddAddress:
      return ErrorCode::kOddAddress;
    case status::kPhaseError:
      return ErrorCode::kPhaseError;
    case status::kAddressMissing:
      return ErrorCode::kAddressMissing;
    case status::kLabelError:
      return ErrorCode::kLabelError;
    case status::kFailError:
      return ErrorCode::kFailDirective;
    case status::kFileError:
      return ErrorCode::kIncludeNotFound;
    default:
      return ErrorCode::kGeneric;
  }
}

bool ReadFile(const std::filesystem::path& path, std::string* contents) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  *contents = ss.str();
  return true;
}

// Finds `name` in `dir`, also matching file names case-insensitively (the
// original ran on Windows, where file names are not case-sensitive).
bool ReadFromDir(const std::filesystem::path& dir, const std::string& name, std::string* contents) {
  std::filesystem::path p = dir / name;
  if (ReadFile(p, contents))
    return true;
  std::error_code ec;
  std::filesystem::path parent = p.parent_path().empty() ? "." : p.parent_path();
  std::string leaf = p.filename().string();
  auto lower = [](std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
  };
  for (const auto& e : std::filesystem::directory_iterator(parent, ec))
    if (lower(e.path().filename().string()) == lower(leaf))
      return ReadFile(e.path(), contents);
  return false;
}

}  // namespace

Assembler::Assembler() = default;
Assembler::~Assembler() = default;

void Assembler::AddIncludePath(const std::string& path) {
  include_paths_.push_back(path);
}

void Assembler::ConfigureListing(const std::string& filename, const std::string& date,
                                 int /*page_length*/, bool cex_flag) {
  source_filename_ = filename;
  created_on_ = date;
  options_.cex = cex_flag;
}

bool Assembler::ReadInclude(const std::string& name, std::string* contents) const {
  if (file_reader_) {
    *contents = file_reader_(name);
    return !contents->empty();
  }
  for (const std::string& dir : include_paths_)
    if (ReadFromDir(dir, name, contents))
      return true;
  if (!source_filename_.empty()) {
    std::filesystem::path dir = std::filesystem::path(source_filename_).parent_path();
    if (!dir.empty() && ReadFromDir(dir, name, contents))
      return true;
  }
  return ReadFromDir(".", name, contents);
}

AssemblyResult Assembler::Assemble(const std::string& source) {
  // The original read Windows text files; accept CR LF and LF.
  std::string text;
  text.reserve(source.size() + 1);
  for (char c : source)
    if (c != '\r')
      text.push_back(c);

  core::CoreOptions co;
  co.generate_list = options_.generate_listing;
  co.generate_srec = options_.generate_srecord;
  co.bitfield = options_.bitfield;
  co.cre = options_.cre;
  co.cex = options_.cex;
  co.mex = options_.mex;
  co.sex = options_.sex;
  co.warnings = options_.warnings;
  co.tab_type = options_.fixed_tabs ? core::Fixed : core::Assembly;
  co.tab_size = options_.tab_size;
  co.created_on = created_on_;
  co.file_name = source_filename_;

  // Edit68K saves editor settings as comment lines at the end of the file
  // ("*~Tab type~1~", "*~Tab size~4~", "*~Font name~...", "*~Font size~...")
  // and removes them before assembling, applying the tab settings to the
  // listing (textS.cpp).
  {
    std::vector<std::string> lines;
    std::istringstream ls(text);
    for (std::string l; std::getline(ls, l);)
      lines.push_back(l);
    bool changed = false;
    int checked = 0;
    for (int i = static_cast<int>(lines.size()) - 1; i >= 0 && checked < 5; --i, ++checked) {
      const std::string& l = lines[i];
      if (l.rfind("*~Tab size~", 0) == 0) {
        // strtol, not atoi: out-of-range values are defined (clamped to int).
        const long ts = std::strtol(l.c_str() + 11, nullptr, 10);
        co.tab_size = static_cast<int>(std::clamp<long>(ts, 1, INT_MAX));
        lines.erase(lines.begin() + i);
        changed = true;
      } else if (l.rfind("*~Tab type~", 0) == 0) {
        co.tab_type = (l.size() > 11 && l[11] == '0') ? core::Assembly : core::Fixed;
        lines.erase(lines.begin() + i);
        changed = true;
      } else if (l.rfind("*~Font size~", 0) == 0 || l.rfind("*~Font name~", 0) == 0) {
        lines.erase(lines.begin() + i);
        changed = true;
      }
    }
    if (changed) {
      text.clear();
      for (const std::string& l : lines)
        text += l + "\n";
    }
  }

  core::AsmCore core(co, [this](const std::string& name, std::string* contents) {
    return ReadInclude(name, contents);
  });
  core.AssembleSource(text);

  AssemblyResult result;
  listing_ = core.Listing();
  result.srecord = core.SRecords();
  result.start_address = core.StartAddress();
  result.warning_count = core.WarningCount();
  result.success = core.ErrorCount() == 0;

  for (const core::CoreMessage& m : core.Messages()) {
    AssemblyError e;
    e.line_number = m.line;
    e.status = m.code;
    e.file = m.file;
    e.severity = status::IsError(m.code) ? ErrorSeverity::kError : ErrorSeverity::kWarning;
    e.code = CodeForStatus(m.code);
    // "ERROR: Invalid syntax" -> "Invalid syntax"
    std::string msg = m.text;
    for (const char* prefix : {"ERROR: ", "WARNING: "}) {
      if (msg.rfind(prefix, 0) == 0) {
        msg.erase(0, std::char_traits<char>::length(prefix));
        break;
      }
    }
    e.message = msg;
    result.errors.push_back(std::move(e));
  }

  const auto& image = core.Image();
  if (!image.empty()) {
    result.org = image.begin()->first;
    uint32_t last = image.rbegin()->first;
    result.code.assign(static_cast<size_t>(last - result.org) + 1, 0);
    for (const auto& [addr, byte] : image)
      result.code[addr - result.org] = byte;
  }

  for (const auto& s : core.Symbols())
    result.symbols.push_back({s.name, s.value, s.flags});
  return result;
}

}  // namespace easym68k::asm_
