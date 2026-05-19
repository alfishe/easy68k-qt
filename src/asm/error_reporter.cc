// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Assembler error reporter — ported from EASy68K/Edit68K/ERROR.CPP + asm.h

#include "easym68k/asm/error_codes.h"

namespace easym68k::asm_ {

const char* ErrorCodeMessage(ErrorCode code) {
  switch (code) {
    case ErrorCode::kGeneric:
      return "assembly error";
    // Data range
    case ErrorCode::kInv8BitData:
      return "INV_8_BIT_DATA: immediate byte value out of range";
    case ErrorCode::kInv16BitData:
      return "INV_16_BIT_DATA: immediate value out of 16-bit range";
    case ErrorCode::kInvMoveQuickConst:
      return "INV_MOVE_QUICK_CONST: MOVEQ immediate must be -128..127";
    case ErrorCode::kInvQuickConst:
      return "INV_QUICK_CONST: quick constant must be 1-8";
    case ErrorCode::kInvShiftCount:
      return "INV_SHIFT_COUNT: shift count must be 1-8";
    case ErrorCode::kInvVectorNum:
      return "INV_VECTOR_NUM: TRAP vector must be 0-15";
    case ErrorCode::kInvBranchDisp:
      return "INV_BRANCH_DISP: branch displacement out of range";
    case ErrorCode::kInvOffset:
      return "INV_OFFSET: displacement out of 16-bit range";
    // Addressing / EA
    case ErrorCode::kInvDisp:
      return "INV_DISP: displacement out of range";
    case ErrorCode::kInvAbsAddress:
      return "INV_ABS_ADDRESS: absolute short address out of 16-bit range";
    case ErrorCode::kInvEa:
      return "INV_EA: invalid addressing mode";
    // Size
    case ErrorCode::kInvSizeCode:
      return "INV_SIZE_CODE: invalid size specifier";
    case ErrorCode::kMissingSizeCode:
      return "MISSING_SIZE_CODE: size specifier required";
    case ErrorCode::kIllegalSizeCode:
      return "ILLEGAL_SIZE_CODE: size not valid for this instruction";
    // Instruction
    case ErrorCode::kInvOp:
      return "INV_OP: invalid operation";
    case ErrorCode::kInvRegister:
      return "INV_REGISTER: invalid register";
    case ErrorCode::kUnknownInstruction:
      return "Unknown instruction";
    // Symbol
    case ErrorCode::kInvLabel:
      return "INV_LABEL: invalid label";
    case ErrorCode::kInvSymbol:
      return "INV_SYMBOL: invalid symbol";
    case ErrorCode::kMultDefd:
      return "MULT_DEFD: symbol multiply defined";
    case ErrorCode::kUndefSymbol:
      return "UNDEF_SYMBOL: undefined symbol";
    case ErrorCode::kDivByZero:
      return "DIV_BY_ZERO: division by zero in expression";
    // Directive
    case ErrorCode::kMissingOperand:
      return "missing required operand";
    case ErrorCode::kDcbNegCount:
      return "DCB count must be non-negative";
    case ErrorCode::kIncludeCycle:
      return "INCLUDE cycle detected";
    case ErrorCode::kIncludeNotFound:
      return "INCLUDE: cannot open file";
    case ErrorCode::kIncbinNotFound:
      return "INCBIN: cannot open file";
    case ErrorCode::kFailDirective:
      return "FAIL";
    case ErrorCode::kMissingLabel:
      return "directive requires a label";
    case ErrorCode::kSectionOutOfRange:
      return "SECTION number must be 0-15";
    case ErrorCode::kMacroNoName:
      return "MACRO requires a name (label)";
    case ErrorCode::kMacroNoEndm:
      return "MACRO without matching ENDM";
    case ErrorCode::kMacroNested:
      return "Nested MACRO definition not allowed";
    case ErrorCode::kEndmOutside:
      return "ENDM outside macro";
    case ErrorCode::kMexitOutside:
      return "MEXIT outside macro";
    case ErrorCode::kMacroRecursion:
      return "macro recursion limit exceeded";
    // Conditional
    case ErrorCode::kElseWithoutIf:
      return "ELSE without matching IF";
    case ErrorCode::kMultipleElse:
      return "Multiple ELSE in same conditional block";
    case ErrorCode::kEndcWithoutIf:
      return "ENDC without matching IF";
    case ErrorCode::kLabelOnCond:
      return "Label not permitted on conditional directive";
    case ErrorCode::kCondMismatch:
      return "mismatched structured control flow";
    // Warnings
    case ErrorCode::kOddAddress:
      return "ODD_ADDRESS: address is odd — auto-aligned to even";
    case ErrorCode::kInvMemory:
      return "INV_MEMORY: invalid memory reference";
    case ErrorCode::kTruncation:
      return "value truncated to fit field";
    case ErrorCode::kPhaseError:
      return "PHASE_ERROR: value differs between pass 1 and pass 2";
  }
  return "unknown error";
}

ErrorSeverity ErrorCodeSeverity(ErrorCode code) {
  switch (code) {
    case ErrorCode::kOddAddress:
    case ErrorCode::kInvMemory:
    case ErrorCode::kTruncation:
    case ErrorCode::kPhaseError:
      return ErrorSeverity::kWarning;
    default:
      return ErrorSeverity::kError;
  }
}

}  // namespace easym68k::asm_
