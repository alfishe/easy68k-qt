// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// 68000 instruction set of the assembler — a faithful port of the original's
// flavor lists and instruction table (Edit68K/INSTTABL.CPP), its instruction
// building routines (Edit68K/BUILD.CPP, CODEGEN.CPP effAddr()/extWords()) and
// the flavor selection of createCode() (Edit68K/ASSEMBLE.CPP), by Paul McKee
// and Charles Kelly.
//
// The same routines run in both passes: in pass 1 they only advance the
// location counter, in pass 2 they also output object code, so instruction
// sizes are identical in both passes by construction.

#ifndef EASYM68K_ASM_INSTRUCTION_SET_H_
#define EASYM68K_ASM_INSTRUCTION_SET_H_

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "easym68k/asm/lexer.h"

namespace easym68k::asm_ {

// Addressing mode bit masks (asm.h); identical to AddressMode values.
inline constexpr int DnDirect = 0x00001;
inline constexpr int AnDirect = 0x00002;
inline constexpr int AnInd = 0x00004;
inline constexpr int AnIndPost = 0x00008;
inline constexpr int AnIndPre = 0x00010;
inline constexpr int AnIndDisp = 0x00020;
inline constexpr int AnIndIndex = 0x00040;
inline constexpr int AbsShort = 0x00080;
inline constexpr int AbsLong = 0x00100;
inline constexpr int PCDisp = 0x00200;
inline constexpr int PCIndex = 0x00400;
inline constexpr int IMMEDIATE = 0x00800;
inline constexpr int SRDirect = 0x01000;
inline constexpr int CCRDirect = 0x02000;
inline constexpr int USPDirect = 0x04000;
inline constexpr int SFCDirect = 0x08000;
inline constexpr int DFCDirect = 0x10000;
inline constexpr int VBRDirect = 0x20000;

// Size codes (asm.h).
inline constexpr int BYTE_SIZE = 1;
inline constexpr int WORD_SIZE = 2;
inline constexpr int LONG_SIZE = 4;
inline constexpr int SHORT_SIZE = 8;

// Operand descriptor (asm.h opDescriptor).
struct OpDescriptor {
  int mode = 0;       // addressing mode bit
  int32_t data = 0;   // immediate value, displacement or absolute address
  int32_t field = 0;  // {offset:width} of bit field instructions
  int reg = 0;        // principal register number (0-7)
  int index = 0;      // index register (0-7 = D0-D7, 8-15 = A0-A7)
  int size = 0;       // index register size, or forced size of an immediate
  bool back_ref = true;
};

// State shared by the building routines: the location counter, the pass,
// the object code sink and the line's status (most severe, NEWERROR).
struct BuildContext {
  uint32_t loc = 0;
  bool pass2 = false;
  // Writes `data` (WORD_SIZE: low 16 bits, LONG_SIZE: 32 bits) at `loc`
  // without advancing it (CODEGEN.CPP output()).
  std::function<void(int32_t data, int size)> output;
  int status = 0;
};

using BuildFn = void (*)(int mask, int size, OpDescriptor* source, OpDescriptor* dest,
                         BuildContext& ctx);

// One "flavor" of an instruction (asm.h flavor).
struct Flavor {
  int source;  // legal source addressing modes (0 = no operands)
  int dest;    // legal destination addressing modes (0 = no destination)
  int sizes;   // legal size codes
  BuildFn exec;
  uint16_t bytemask, wordmask, longmask;
};

struct Instruction {
  const char* mnemonic;
  const Flavor* flavors;
  size_t flavor_count;
};

// Looks up an instruction assembled through the flavor table (the original's
// parseFlag instructions); nullptr if `mnemonic` is not one.
const Instruction* LookupInstruction(const std::string& mnemonic);

// True if the instruction's first flavor is a bit field instruction (needs
// OPT BITFIELD in the original).
bool IsBitFieldInstruction(const Instruction& instruction);
// True if `flavor` is built by the bit field routine (bitField()).
bool IsBitFieldFlavor(const Flavor& flavor);

// All flavor-table instructions, in alphabetical order.
const std::vector<Instruction>& AllInstructions();

// ASSEMBLE.CPP pickMask(): the skeleton mask for `size` (see the original's
// dangling else in instruction_set.cc).
uint16_t PickMask(int size, const Flavor& flavor, int& status);

// Original size code for a parsed size suffix (0 when none).
int SizeCode(SizeSpec size);

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_INSTRUCTION_SET_H_
