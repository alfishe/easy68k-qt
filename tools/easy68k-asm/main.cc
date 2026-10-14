// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// easy68k-asm — command-line front end for the EASy68K-Qt assembler.
// Writes <base>.L68 (listing) and <base>.S68 (S-records) like Edit68K, with
// the same options as the reference driver (tools/reference/edit68k):
//
// Usage: easy68k-asm [--no-list] [--no-srec] [--cex] [--no-bitfield]
//                    [--no-cre] [--mex] [--sex] [--no-warnings]
//                    <source.x68> [-o <base>]
//
// Messages go to stderr as "line N: <text>"; the exit status is 1 when the
// assembly has errors. Include files are looked up relative to the source.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "easym68k/asm/assembler.h"

namespace {

int Usage() {
  std::fprintf(stderr,
               "usage: easy68k-asm [--no-list] [--no-srec] [--cex] [--no-bitfield] [--no-cre]\n"
               "                   [--mex] [--sex] [--no-warnings] <source.x68> [-o <base>]\n");
  return 2;
}

std::string StripExtension(const std::string& path) {
  size_t slash = path.find_last_of("/\\");
  size_t dot = path.find_last_of('.');
  if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
    return path.substr(0, dot);
  return path;
}

}  // namespace

int main(int argc, char** argv) {
  std::string source, base;
  easym68k::asm_::AssemblerOptions options;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--no-list")
      options.generate_listing = false;
    else if (a == "--no-srec")
      options.generate_srecord = false;
    else if (a == "--cex")
      options.cex = true;
    else if (a == "--no-bitfield")
      options.bitfield = false;
    else if (a == "--no-cre")
      options.cre = false;
    else if (a == "--mex")
      options.mex = true;
    else if (a == "--sex")
      options.sex = true;
    else if (a == "--no-warnings")
      options.warnings = false;
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

  easym68k::asm_::Assembler assembler;
  const char* date = std::getenv("EASY68K_REF_DATE");
  assembler.ConfigureListing(source, date ? date : "", 60, options.cex);
  assembler.SetOptions(options);
  easym68k::asm_::AssemblyResult r = assembler.Assemble(ss.str());

  // Like Edit68K, the files are written even when the assembly has errors.
  if (options.generate_listing) {
    std::ofstream out(base + ".L68", std::ios::binary);
    out << assembler.GetListing();
  }
  if (options.generate_srecord) {
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
