// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from Edit68K/INSTLOOK.CPP of the original EASy68K (Paul McKee, Charles
// Kelly, Tim Larson). The functions follow the original line by line; the
// original's global state lives in AsmCore (see asm_core.h).

#include "asm_core.h"

#include <algorithm>

namespace easym68k::asm_::core {

/***********************************************************************
 *
 *		INSTLOOKUP.C
 *		Instruction Table Lookup Routine for 68000 Assembler
 *
 *    Function: instLookup()
 *		Parses an instruction and looks it up in the instruction
 *		table. The input to the function is a pointer to the
 *		instruction on a line of assembly code. The routine 
 *		scans the instruction and notes the size code if 
 *		present. It then (binary) searches the instruction 
 *		table for the specified opcode. If it finds the opcode, 
 *		it returns a pointer to the instruction table entry for 
 *		that instruction (via the instPtrPtr argument) as well 
 *		as the size code or 0 if no size was specified (via the 
 *		sizePtr argument). If the opcode is not in the 
 *		instruction table, then the routine returns INV_OPCODE. 
 *		The routine returns an error value via the standard
 *		mechanism. 
 *
 *	 Usage:	char *instLookup(p, instPtrPtr, sizePtr, errorPtr)
 *		char *p;
 *		instruction *(*instPtrPtr);
 *		char *sizePtr;
 *		int *errorPtr;
 *
 *	Errors: SYNTAX
 *		INV_OPCODE
 *		INV_SIZE_CODE
 *
 *      Author: Paul McKee
 *		ECE492    North Carolina State University
 *
 *        Date:	9/24/86
 *
 *    Modified: Charles Kelly
 *              Monroe County Community College
 *              http://www.monroeccc.edu/ckelly
 *
 ************************************************************************/





// INSTTABL.CPP instTable[]: the flavor-table instructions of
// easym68k/asm/instruction_set.h plus the routines called with the raw
// operand text (parseFlag false), in alphabetical order.
const std::vector<AsmCore::instruction>& AsmCore::InstTable() {
  static const std::vector<instruction> table = [] {
    std::vector<instruction> t = {
        {"DBLOOP", nullptr, false, &AsmCore::asmStructure},
        {"DC", nullptr, false, &AsmCore::dc},
        {"DCB", nullptr, false, &AsmCore::dcb},
        {"DS", nullptr, false, &AsmCore::ds},
        {"ELSE", nullptr, false, &AsmCore::asmStructure},
        {"END", nullptr, false, &AsmCore::funct_end},
        {"ENDF", nullptr, false, &AsmCore::asmStructure},
        {"ENDI", nullptr, false, &AsmCore::asmStructure},
        {"ENDW", nullptr, false, &AsmCore::asmStructure},
        {"EQU", nullptr, false, &AsmCore::equ},
        {"FAIL", nullptr, false, &AsmCore::failError},
        {"FOR", nullptr, false, &AsmCore::asmStructure},
        {"IF", nullptr, false, &AsmCore::asmStructure},
        {"INCBIN", nullptr, false, &AsmCore::incbin},
        {"INCLUDE", nullptr, false, &AsmCore::include},
        {"LIST", nullptr, false, &AsmCore::listOn},
        {"MACRO", nullptr, false, &AsmCore::macro},
        {"MEMORY", nullptr, false, &AsmCore::memory},
        {"MOVEM", nullptr, false, &AsmCore::movem},
        {"NOLIST", nullptr, false, &AsmCore::listOff},
        {"OFFSET", nullptr, false, &AsmCore::offset},
        {"OPT", nullptr, false, &AsmCore::opt},
        {"ORG", nullptr, false, &AsmCore::org},
        {"PAGE", nullptr, false, &AsmCore::page},
        {"REG", nullptr, false, &AsmCore::reg},
        {"REPEAT", nullptr, false, &AsmCore::asmStructure},
        {"SECTION", nullptr, false, &AsmCore::section},
        {"SET", nullptr, false, &AsmCore::set},
        {"SIMHALT", nullptr, false, &AsmCore::simhalt},
        {"UNLESS", nullptr, false, &AsmCore::asmStructure},
        {"UNTIL", nullptr, false, &AsmCore::asmStructure},
        {"WHILE", nullptr, false, &AsmCore::asmStructure},
    };
    for (const auto& inst : ::easym68k::asm_::AllInstructions())
      t.push_back({inst.mnemonic, &inst, true, nullptr});
    std::sort(t.begin(), t.end(), [](const instruction& a, const instruction& b) {
      return std::strcmp(a.mnemonic, b.mnemonic) < 0;
    });
    return t;
  }();
  return table;
}


char *AsmCore::instLookup(char *p, const instruction **instPtrPtr, char *sizePtr, int *errorPtr)
{
  char opcode[SIGCHARS+1];
  int i, hi, lo, mid, cmp;
  symbolDef *symbol;
  static const instruction asmMac = {"ASMMACRO", nullptr, false, &AsmCore::asmMacro};

  try {
    /*	printf("InstLookup: Input string is \"%s\"\n", p); */
    i = 0;
    do {
      if (i < SIGCHARS)
        opcode[i++] = *p;
      p++;
    } while (isalnum(*p) || *p == '_' || *p == '-');
    opcode[i] = '\0';
    if (*p == '.')
      if (isspace(p[2]) || !p[2]) {
        if (p[1] == 'B')
          *sizePtr = BYTE_SIZE;
        else if (p[1] == 'W' || isspace(p[1]))  // *ck 12-8-2005
          *sizePtr = WORD_SIZE;
        else if (p[1] == 'L')
          *sizePtr = LONG_SIZE;
        else if (p[1] == 'S')
          *sizePtr = SHORT_SIZE;
        else {
          *sizePtr = 0;
          NEWERROR(*errorPtr, INV_SIZE_CODE);
        }
        p += 2;
      } else {
        NEWERROR(*errorPtr, SYNTAX);
        return NULL;
      }
    else if (!isspace(*p) && *p) {
      NEWERROR(*errorPtr, SYNTAX);
      return NULL;
    }
    else
      *sizePtr = 0;

    lo = 0;
    const std::vector<instruction>& instTable = InstTable();
    hi = static_cast<int>(instTable.size()) - 1;

    // search for opcode in instTable
    do {
      mid = (hi + lo) / 2;
      cmp = strcmp(opcode, instTable[mid].mnemonic);
      if (cmp > 0)
        lo = mid + 1;
      else if (cmp < 0)
        hi = mid - 1;
    } while (cmp && (hi >= lo));

    // if opcode found
    if (!cmp) {
      // if bitfield instruction and BITflag is false
      if (instTable[mid].flavors &&
          ::easym68k::asm_::IsBitFieldInstruction(*instTable[mid].flavors) && !BITflag) {
        NEWERROR(*errorPtr, INV_OPCODE);        // error invalid opcode
        return NULL;
      }
      *instPtrPtr = &instTable[mid];
      return p;

    // else, opcode not found
    } else {

      // search for matching macro definition in symbol table
      symbol = lookup(opcode, false, errorPtr);
      if ( (*errorPtr < ERRORN) && (symbol->flags & MACRO_SYM) ) { // if found
        if(pass2 && !(symbol->flags & BACKREF))  // if forward reference
          NEWERROR(*errorPtr, FORWARD_REF);     // warning
        *instPtrPtr = &asmMac;    // point to asmMac function description
        macroFP = symbol->value;  // get file pointer to macro
        return p;                 // return pointer to macro parameters
      } else {

        // not a macro call either, so must be invalid opcode
        NEWERROR(*errorPtr, INV_OPCODE);
        return NULL;
      }
    }
  }
  catch( ... ) {
    NEWERROR(*errorPtr, EXCEPTION);
    snprintf(buffer, sizeof(buffer), "ERROR: An exception occurred in routine 'instLookup'. \n");
    return NULL;
  }
  //return NORMAL;
}








}  // namespace easym68k::asm_::core
