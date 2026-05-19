// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Assembler error codes — ported from EASy68K/Edit68K/asm.h

#ifndef EASYM68K_ASM_ERROR_CODES_H_
#define EASYM68K_ASM_ERROR_CODES_H_

namespace easym68k::asm_ {

enum class ErrorSeverity { kError, kWarning };

// Specific error/warning codes for the assembler.
// kGeneric (0) is used for messages emitted without a specific code.
enum class ErrorCode {
  kGeneric = 0,

  // Data range errors
  kInv8BitData,        // INV_8_BIT_DATA
  kInv16BitData,       // INV_16_BIT_DATA
  kInvMoveQuickConst,  // INV_MOVE_QUICK_CONST — MOVEQ immediate out of -128..127
  kInvQuickConst,      // INV_QUICK_CONST — ADDQ/SUBQ count not 1-8
  kInvShiftCount,      // INV_SHIFT_COUNT — shift count not 1-8
  kInvVectorNum,       // INV_VECTOR_NUM — TRAP vector not 0-15
  kInvBranchDisp,      // INV_BRANCH_DISP — branch displacement out of range
  kInvOffset,          // INV_OFFSET — LINK/UNLK displacement out of range

  // Addressing / EA errors
  kInvDisp,        // INV_DISP — register-indirect displacement out of range
  kInvAbsAddress,  // INV_ABS_ADDRESS — absolute-short address out of 16-bit range
  kInvEa,          // INV_EA — addressing mode not permitted for this operand

  // Size errors
  kInvSizeCode,      // INV_SIZE_CODE — unrecognised size specifier
  kMissingSizeCode,  // MISSING_SIZE_CODE — size required but absent
  kIllegalSizeCode,  // ILLEGAL_SIZE_CODE — size not valid for this instruction

  // Instruction errors
  kInvOp,               // INV_OP — invalid operation
  kInvRegister,         // INV_REGISTER — invalid register
  kUnknownInstruction,  // unknown instruction mnemonic

  // Symbol errors
  kInvLabel,     // INV_LABEL — invalid label syntax
  kInvSymbol,    // INV_SYMBOL — invalid symbol reference
  kMultDefd,     // MULT_DEFD — multiply-defined symbol
  kUndefSymbol,  // UNDEF_SYMBOL — undefined symbol
  kDivByZero,    // DIV_BY_ZERO — division by zero in expression

  // Directive errors
  kMissingOperand,     // directive requires an operand
  kDcbNegCount,        // DCB count must be non-negative
  kIncludeCycle,       // INCLUDE cycle detected
  kIncludeNotFound,    // INCLUDE: cannot open file
  kIncbinNotFound,     // INCBIN: cannot open file
  kFailDirective,      // FAIL directive
  kMissingLabel,       // directive (EQU/SET/REG) requires a label
  kSectionOutOfRange,  // SECTION number must be 0-15
  kMacroNoName,        // MACRO without a name label
  kMacroNoEndm,        // MACRO without matching ENDM
  kMacroNested,        // nested MACRO definition not allowed
  kEndmOutside,        // ENDM outside a macro body
  kMexitOutside,       // MEXIT outside a macro body
  kMacroRecursion,     // macro recursion limit exceeded

  // Conditional assembly errors
  kElseWithoutIf,  // ELSE without matching IF
  kMultipleElse,   // Multiple ELSE in same conditional block
  kEndcWithoutIf,  // ENDC without matching IF
  kLabelOnCond,    // label not permitted on conditional directive
  kCondMismatch,   // structured control flow mismatch

  // Warnings (severity == kWarning)
  kOddAddress,  // ODD_ADDRESS — address is odd, auto-aligned
  kInvMemory,   // INV_MEMORY — invalid memory range
  kTruncation,  // value truncated to fit field
  kPhaseError,  // value differs between pass 1 and pass 2
};

// Returns the canonical short message for a given error code.
// The returned pointer is to a string literal; lifetime is static.
const char* ErrorCodeMessage(ErrorCode code);

// Returns whether a given code is a warning or an error.
ErrorSeverity ErrorCodeSeverity(ErrorCode code);

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_ERROR_CODES_H_
