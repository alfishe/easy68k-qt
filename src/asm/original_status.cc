// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Diagnostic texts of the original assembler — ported from
// Edit68K/ERROR.CPP printError().

#include "easym68k/asm/original_status.h"

namespace easym68k::asm_::status {

const char* Message(int code) {
  switch (code) {
    case kNumberTooBig:
      return "Numeric constant exceeds 32 bits";
    case kAsciiTooBig:
      return "ASCII constant exceeds 4 characters";
    case kIncomplete:
      return "Evaluation of expression could not be completed";
    case kForcingShort:
      return "Forcing SHORT addressing disables range checking of extension word";
    case kOddAddress:
      return "Origin value is odd (Location counter set to next highest address)";
    case kEndMissing:
      return "END directive missing, starting address not set";
    case kAddressMissing:
      return "Address expected";
    case kThenExpected:
      return "THEN expected";
    case kDoExpected:
      return "DO expected";
    case kForwardRef:
      return "Forward reference may cause 'Symbol value differs...' error";
    case kLabelTooLong:
      return "Label too long";
    case kSyntax:
      return "Invalid syntax";
    case kUndefined:
      return "Undefined symbol";
    case kDivByZero:
      return "Division by zero attempted";
    case kInvOpcode:
      return "Invalid opcode";
    case kInvSizeCode:
      return "Invalid size code";
    case kInvAddrMode:
      return "Invalid addressing mode";
    case kMultipleDefs:
      return "Symbol defined more than once";
    case kNoEndm:
      return "ENDM expected";
    case kTooManyArgs:
      return "Too many arguments";
    case kInvalidArg:
      return "Invalid argument";
    case kPhaseError:
      return "Symbol value differs between first and second pass";
    case kInvQuickConst:
      return "Quick immediate data range must be 1 to 8";
    case kInvMoveQuickConst:
      return "Quick immediate data range exceeds 1 byte";
    case kInvVectorNum:
      return "Invalid vector number";
    case kInvBranchDisp:
      return "Branch instruction displacement is out of range or invalid";
    case kLabelRequired:
      return "Label required with this directive";
    case kInvDisp:
      return "Displacement out of range";
    case kInvAbsAddress:
      return "Absolute address exceeds 16 bits";
    case kInv8BitData:
      return "Immediate data exceeds 8 bits";
    case kInv16BitData:
      return "Immediate data exceeds 16 bits";
    case kNotRegList:
      return "Symbol is not a register list symbol";
    case kRegListSpec:
      return "Register list symbol used in an expression";
    case kRegListUndef:
      return "Register list symbol not previously defined";
    case kInvShiftCount:
      return "Invalid constant shift count";
    case kInvOperator:
      return "Invalid operator";
    case kInvForwardRef:
      return "Forward references not allowed with this directive";
    case kInvLength:
      return "Invalid block length";
    case kCommaExpected:
      return "Comma expected";
    case kFileError:
      return "Unable to access specified file";
    case kMacroNest:
      return "Nested Macro calls are too many levels deep";
    case kNoIf:
      return "No matching IF statement was found";
    case kNoWhile:
      return "No matching WHILE statement was found";
    case kNoRepeat:
      return "No matching REPEAT statement was found";
    case kNoDbloop:
      return "No matching DBLOOP statement was found";
    case kNoFor:
      return "No matching FOR statement was found";
    case kEndiExpected:
      return "IF without ENDI";
    case kEndwExpected:
      return "WHILE without ENDW";
    case kEndfExpected:
      return "FOR without ENDF";
    case kRepeatExpected:
      return "UNTIL without REPEAT";
    case kDbloopExpected:
      return "UNLESS without DBLOOP";
    case kLabelError:
      return "Label is not allowed";
    case kBadBitfield:
      return "Invalid bitfield";
    case kIllegalSymbol:
      return "Illegal symbol";
    default:
      return nullptr;
  }
}

}  // namespace easym68k::asm_::status
