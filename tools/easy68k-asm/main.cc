// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// easy68k-asm — command-line front end for the EASy68K-Qt assembler.
// Writes <base>.L68 (listing) and <base>.S68 (S-records) like Edit68K.
//
// Usage: easy68k-asm [--no-list] [--no-srec] [--cex] <source.x68> [-o <base>]

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "easym68k/asm/assembler.h"

namespace {

int Usage() {
  std::fprintf(stderr,
               "usage: easy68k-asm [--no-list] [--no-srec] [--cex] <source.x68> [-o <base>]\n");
  return 2;
}

std::string StripExtension(const std::string& path) {
  size_t slash = path.find_last_of("/\\");
  size_t dot = path.find_last_of('.');
  if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
    return path.substr(0, dot);
  return path;
}

// Edit68K strips its editor settings trailer ("*~Tab size~4~" etc.) from the
// last lines of a source file before assembling (textS.cpp).
std::string StripEditorTrailer(const std::string& text) {
  std::vector<std::string> lines;
  std::istringstream in(text);
  for (std::string l; std::getline(in, l);)
    lines.push_back(l);
  int checked = 0;
  for (int i = static_cast<int>(lines.size()) - 1; i >= 0 && checked < 5; --i, ++checked) {
    const std::string& l = lines[i];
    if (l.rfind("*~Tab size~", 0) == 0 || l.rfind("*~Tab type~", 0) == 0 ||
        l.rfind("*~Font size~", 0) == 0 || l.rfind("*~Font name~", 0) == 0)
      lines.erase(lines.begin() + i);
  }
  std::string out;
  for (const std::string& l : lines)
    out += l + "\n";
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  bool list = true, srec = true, cex = false;
  std::string source, base;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--no-list")
      list = false;
    else if (a == "--no-srec")
      srec = false;
    else if (a == "--cex")
      cex = true;
    else if (a == "-o" && i + 1 < argc)
      base = argv[++i];
    else if (!a.empty() && a[0] == '-')
      return Usage();
    else
      source = a;
  }
  if (source.empty())
    return Usage();
  if (base.empty())
    base = StripExtension(source);

  std::ifstream in(source, std::ios::binary);
  if (!in) {
    std::fprintf(stderr, "cannot open %s\n", source.c_str());
    return 2;
  }
  std::stringstream ss;
  ss << in.rdbuf();
  std::string text;
  for (char c : ss.str())
    if (c != '\r')
      text.push_back(c);
  text = StripEditorTrailer(text);

  easym68k::asm_::Assembler assembler;
  const char* date = std::getenv("EASY68K_REF_DATE");
  assembler.ConfigureListing(source, date ? date : "", 60, cex);
  easym68k::asm_::AssemblyResult r = assembler.Assemble(text);

  if (list) {
    std::ofstream out(base + ".L68", std::ios::binary);
    out << assembler.GetListing();
  }
  if (srec && r.success) {
    std::ofstream out(base + ".S68", std::ios::binary);
    out << r.srecord;
  }
  int errors = 0;
  for (const auto& e : r.errors) {
    std::fprintf(stderr, "line %d: %s\n", e.line_number, e.message.c_str());
    if (e.severity == easym68k::asm_::ErrorSeverity::kError)
      ++errors;
  }
  std::fprintf(stderr, "%d error(s), %d warning(s)\n", errors, r.warning_count);
  return r.success ? 0 : 1;
}
