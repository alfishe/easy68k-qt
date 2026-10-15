// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// easy68k-ref-sim: headless driver around the ORIGINAL Sim68K core, used as
// the reference oracle for simulator parity (tools/reference/compare_sim.sh).
//
//   easy68k-ref-sim [-e] [-b] [-n MAX] [-t] [-o TRACE] [-m ADDR:LEN]... FILE.S68
//
//   -e   exceptions enabled (Sim68K /e; the original default is off)
//   -b   bitfield instructions enabled (Sim68K /b)
//   -n   stop after MAX instructions (default 100000)
//   -t   write one register line per executed instruction
//   -o   trace file (default stderr)
//   -m   dump LEN bytes of memory at ADDR (hex) after the run
//   -i   raise interrupt LEVEL before instruction STEP (1-based): -i STEP:LEVEL
//
// Mirrors TForm1::OpenFile (initSim, erase memory to $FF, clear the toggle
// switch byte, loadSrec) followed by TForm1::runLoop (runprog until halted).
// Program text output (Trap #15) goes to stdout; input comes from stdin.
// The trace format is shared with the port's `easy68k-sim --trace`.

#include <cinttypes>

#include "def.h"
#include "extern.h"
#include "proto.h"

FILE* g_trace_out = stderr;

extern int switchLoc;  // hardwareu.cpp (stubs.cc)

namespace {

void PrintRegs(const char* tag) {
  std::fprintf(g_trace_out, "%s PC=%08X SR=%04X", tag, (unsigned)PC, (unsigned)(SR & 0xFFFF));
  for (int i = 0; i < 8; i++)
    std::fprintf(g_trace_out, " D%d=%08X", i, (unsigned)D[i]);
  for (int i = 0; i < 7; i++)
    std::fprintf(g_trace_out, " A%d=%08X", i, (unsigned)A[i]);
  std::fprintf(g_trace_out, " US=%08X SS=%08X CYC=%" PRIu64 "\n", (unsigned)A[7], (unsigned)A[8],
               (uint64_t)cycles);
}

uint32_t MemoryHash() {
  uint32_t h = 2166136261u;  // FNV-1a over the whole 68000 address space
  for (int i = 0; i < MEMSIZE; i++) {
    h ^= (unsigned char)memory[i];
    h *= 16777619u;
  }
  return h;
}

void DumpMemory(unsigned addr, unsigned len) {
  for (unsigned row = 0; row < len; row += 16) {
    std::fprintf(g_trace_out, "MEM %08X:", addr + row);
    for (unsigned i = row; i < len && i < row + 16; i++)
      std::fprintf(g_trace_out, " %02X", (unsigned char)memory[(addr + i) & ADDRMASK]);
    std::fprintf(g_trace_out, "\n");
  }
}

int Usage() {
  std::fprintf(stderr,
               "usage: easy68k-ref-sim [-e] [-b] [-n MAX] [-t] [-o TRACE] [-m ADDR:LEN]... "
               "[-i STEP:LEVEL]... FILE.S68\n");
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  bool opt_exceptions = false, opt_bitfield = false, opt_trace = false;
  long max_steps = 100000;
  const char* file = nullptr;
  std::vector<std::pair<unsigned, unsigned>> dumps;
  std::vector<std::pair<long, int>> irqs;  // (step, level)
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "-e") {
      opt_exceptions = true;
    } else if (a == "-b") {
      opt_bitfield = true;
    } else if (a == "-t") {
      opt_trace = true;
    } else if (a == "-n" && i + 1 < argc) {
      max_steps = std::strtol(argv[++i], nullptr, 0);
    } else if (a == "-o" && i + 1 < argc) {
      g_trace_out = std::fopen(argv[++i], "w");
      if (!g_trace_out) {
        std::perror(argv[i]);
        return 2;
      }
    } else if (a == "-i" && i + 1 < argc) {
      long step = 0;
      int level = 0;
      if (std::sscanf(argv[++i], "%ld:%d", &step, &level) != 2 || level < 1 || level > 7)
        return Usage();
      irqs.emplace_back(step, level);
    } else if (a == "-m" && i + 1 < argc) {
      unsigned addr = 0, len = 0;
      if (std::sscanf(argv[++i], "%x:%x", &addr, &len) != 2)
        return Usage();
      dumps.emplace_back(addr, len);
    } else if (!a.empty() && a[0] != '-' && !file) {
      file = argv[i];
    } else {
      return Usage();
    }
  }
  if (!file)
    return Usage();

  // put()/value_of() (UTILS.CPP) treat an EA pointer in [&D[0], &inst) as a
  // register and anything else as memory; var.h relies on the globals being
  // laid out in declaration order.
  const char* lo = (const char*)&D[0];
  const char* hi = (const char*)&inst;
  const void* regs[] = {&A[0], &PC, &SR, &global_temp, &EV1, &source, &dest, &result};
  for (const void* r : regs) {
    if ((const char*)r < lo || (const char*)r >= hi) {
      std::fprintf(stderr, "easy68k-ref-sim: register globals are not contiguous\n");
      return 2;
    }
  }

  // TForm1::FormCreate: reserve memory, erase to $FF; defaultSettings().
  memory = new char[MEMSIZE];
  for (int i = 0; i < MEMSIZE; i++)
    memory[i] = 0xFF;
  ElogFlag = 0;
  OlogFlag = 0;
  logging = false;
  exceptions = opt_exceptions;
  bitfield = opt_bitfield;
  simhalt_on = true;

  // TForm1::OpenFile.
  initSim();
  for (int i = 0; i < MEMSIZE; i++)
    memory[i] = 0xFF;
  memory[switchLoc] = 0x00;
  simhalt_on = true;
  if (loadSrec(const_cast<char*>(file)) != false) {
    std::fprintf(g_trace_out, "END LOADFAIL steps=0\n");
    return 1;
  }
  OLD_PC = PC;
  PrintRegs("INIT");

  // Run (TForm1::Run1Click -> runLoop).
  trace = false;
  sstep = false;
  runMode = true;
  long steps = 0;
  const char* reason = "LIMIT";
  while (steps < max_steps) {
    for (const auto& q : irqs)
      if (q.first == steps + 1)
        irq |= 1 << (q.second - 1);  // hardwareu.cpp: request IRQ level
    runprog();
    steps++;
    if (opt_trace)
      PrintRegs("STEP");
    if (halt) {
      reason = "HALT";
      break;
    }
    if (stopInstruction) {
      reason = "STOP";
      break;
    }
    if (!runMode) {
      reason = "BREAK";
      break;
    }
  }
  std::fflush(stdout);
  std::fprintf(g_trace_out, "END %s steps=%ld\n", reason, steps);
  PrintRegs("FINAL");
  std::fprintf(g_trace_out, "HASH %08X\n", MemoryHash());
  for (const auto& d : dumps)
    DumpMemory(d.first, d.second);
  if (g_trace_out != stderr)
    std::fclose(g_trace_out);
  return 0;
}
