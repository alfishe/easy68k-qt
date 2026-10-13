// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// easy68k-ref-asm — command-line driver for the ORIGINAL EASy68K Edit68K
// assembler core (Borland sources from the EASy68K repository, compiled
// unmodified against compat/). Used as a reference oracle to generate golden
// .L68/.S68 files and to check the port's assembler for parity.
//
// Mirrors TMain::mnuDoAssemblerClick() (mainS.cpp): set the option flags,
// save the source to a temporary file and call assembleFile().
//
// Usage: easy68k-ref-asm [options] <source.x68> [-o <output base path>]
//   --no-list --no-srec --cex --no-bitfield --no-cre --mex --sex --no-warnings
// Defaults follow optionsS.dfm: list, S-record, bitfield, cross-reference and
// warnings on; constants/macro/structured expansion off.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "gui_stubs.h"
#include "vcl.h"

// Globals defined by the original sources (GLOBALS.CPP).
extern bool listFlag, objFlag, CEXflag, BITflag, CREflag, MEXflag, SEXflag, WARflag;
extern int errorCount, warningCount;

int assembleFile(char fileName[], char tempName[], AnsiString workName);

// GUI stand-in instances.
static TApplication g_application;
TApplication* Application = &g_application;
static TOptionsStub g_options;
TOptionsStub* Options = &g_options;
static TTextStuff g_active;
TTextStuff* TextStuff = &g_active;
static TMainStub g_main;
TMainStub* Main = &g_main;
static TAssemblerBoxStub g_assembler_box;
TAssemblerBoxStub* AssemblerBox = &g_assembler_box;

static int Usage() {
  std::fprintf(stderr,
               "usage: easy68k-ref-asm [--no-list] [--no-srec] [--cex] [--no-bitfield] [--no-cre]\n"
               "                       [--mex] [--sex] [--no-warnings] <source.x68> [-o <base>]\n");
  return 2;
}

int main(int argc, char** argv) {
  bool list = true, srec = true, cex = false, bit = true, cre = true, mex = false, sex = false,
       war = true;
  std::string source, base;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--no-list")
      list = false;
    else if (a == "--no-srec")
      srec = false;
    else if (a == "--cex")
      cex = true;
    else if (a == "--no-bitfield")
      bit = false;
    else if (a == "--no-cre")
      cre = false;
    else if (a == "--mex")
      mex = true;
    else if (a == "--sex")
      sex = true;
    else if (a == "--no-warnings")
      war = false;
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
    base = ChangeFileExt(source.c_str(), "").str();

  // Read the source and normalise CR LF (the original read Windows text files).
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

  // Edit68K appends editor settings as comment lines ("*~Tab type~1~",
  // "*~Tab size~4~", font lines) and strips them before assembling, applying
  // the tab settings to the listing (textS.cpp). Do the same.
  {
    std::vector<std::string> lines;
    std::istringstream ls(text);
    for (std::string l; std::getline(ls, l);)
      lines.push_back(l);
    int checked = 0;
    for (int i = static_cast<int>(lines.size()) - 1; i >= 0 && checked < 5; --i, ++checked) {
      const std::string& l = lines[i];
      if (l.rfind("*~Tab size~", 0) == 0) {
        g_active.Project.TabSize = std::atoi(l.c_str() + 11);
        lines.erase(lines.begin() + i);
      } else if (l.rfind("*~Tab type~", 0) == 0) {
        g_active.Project.TabType = (l.size() > 11 && l[11] == '0') ? 0 : 1;
        lines.erase(lines.begin() + i);
      } else if (l.rfind("*~Font size~", 0) == 0 || l.rfind("*~Font name~", 0) == 0) {
        lines.erase(lines.begin() + i);
      }
    }
    text.clear();
    for (const std::string& l : lines)
      text += l + "\n";
  }

  std::string src_tmp = base + ".EASy68Ks.tmp";
  std::string mac_tmp = base + ".EASy68Km.tmp";
  {
    std::ofstream out(src_tmp, std::ios::binary);
    out << text;
  }

  g_options.chkGenList->Checked = list;
  g_options.chkGenSRec->Checked = srec;
  listFlag = list;
  objFlag = srec;
  CEXflag = cex;
  BITflag = bit;
  CREflag = cre;
  MEXflag = mex;
  SEXflag = sex;
  WARflag = war;

  g_active.Project.CurrentFile = source.c_str();
  g_main.ActiveMDIChild = &g_active;

  std::string work = base + ".X68";  // assembleFile() derives .L68/.S68 from this
  int rc = assembleFile(const_cast<char*>(src_tmp.c_str()), const_cast<char*>(mac_tmp.c_str()),
                        work.c_str());
  std::remove(src_tmp.c_str());
  std::remove(mac_tmp.c_str());

  for (TListItem* item : g_active.Messages->Items->items) {
    std::fprintf(stderr, "line %s:", item->Caption.c_str());
    for (const AnsiString& s : item->SubItems->items)
      std::fprintf(stderr, " %s", s.c_str());
    std::fprintf(stderr, "\n");
  }
  std::fprintf(stderr, "%d error(s), %d warning(s)\n", errorCount, warningCount);
  return (rc != 0 || errorCount > 0) ? 1 : 0;
}
