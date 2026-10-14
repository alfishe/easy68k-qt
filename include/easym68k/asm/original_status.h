// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Status codes and diagnostic texts of the original assembler — ported from
// Edit68K/asm.h and Edit68K/ERROR.CPP (Paul McKee / Charles Kelly).
//
// A status is 12 bits with the severity in the upper 4 bits. The original
// keeps only the most severe status per source line (NEWERROR).

#ifndef EASYM68K_ASM_ORIGINAL_STATUS_H_
#define EASYM68K_ASM_ORIGINAL_STATUS_H_

namespace easym68k::asm_::status {

inline constexpr int kOk = 0x00;

// Severe errors
inline constexpr int kSevere = 0x400;
inline constexpr int kSyntax = 0x401;
inline constexpr int kInvOpcode = 0x402;
inline constexpr int kInvAddrMode = 0x403;
inline constexpr int kLabelRequired = 0x404;
inline constexpr int kNoEndm = 0x405;
inline constexpr int kTooManyArgs = 0x406;
inline constexpr int kInvalidArg = 0x407;
inline constexpr int kCommaExpected = 0x408;
inline constexpr int kPhaseError = 0x409;
inline constexpr int kFileError = 0x40A;
inline constexpr int kMacroNest = 0x40B;
inline constexpr int kNoIf = 0x40C;
inline constexpr int kNoWhile = 0x40D;
inline constexpr int kNoRepeat = 0x40E;
inline constexpr int kNoFor = 0x40F;
inline constexpr int kEndiExpected = 0x410;
inline constexpr int kEndwExpected = 0x411;
inline constexpr int kEndfExpected = 0x412;
inline constexpr int kRepeatExpected = 0x413;
inline constexpr int kLabelError = 0x414;
inline constexpr int kNoDbloop = 0x415;
inline constexpr int kDbloopExpected = 0x416;
inline constexpr int kBadBitfield = 0x417;
inline constexpr int kIllegalSymbol = 0x418;

// Errors
inline constexpr int kError = 0x300;
inline constexpr int kUndefined = 0x301;
inline constexpr int kDivByZero = 0x302;
inline constexpr int kMultipleDefs = 0x303;
inline constexpr int kRegMultDefs = 0x304;
inline constexpr int kRegListUndef = 0x305;
inline constexpr int kInvForwardRef = 0x306;
inline constexpr int kInvLength = 0x307;

// Minor errors
inline constexpr int kMinor = 0x200;
inline constexpr int kInvSizeCode = 0x201;
inline constexpr int kInvQuickConst = 0x202;
inline constexpr int kInvMoveQuickConst = 0x203;
inline constexpr int kInvVectorNum = 0x204;
inline constexpr int kInvBranchDisp = 0x205;
inline constexpr int kInvDisp = 0x206;
inline constexpr int kInvAbsAddress = 0x207;
inline constexpr int kInv8BitData = 0x208;
inline constexpr int kInv16BitData = 0x209;
inline constexpr int kNotRegList = 0x20A;
inline constexpr int kRegListSpec = 0x20B;
inline constexpr int kInvShiftCount = 0x20C;
inline constexpr int kInvOperator = 0x20D;
inline constexpr int kFailError = 0x20E;

// Warnings
inline constexpr int kWarning = 0x100;
inline constexpr int kAsciiTooBig = 0x101;
inline constexpr int kNumberTooBig = 0x102;
inline constexpr int kIncomplete = 0x103;
inline constexpr int kForcingShort = 0x104;
inline constexpr int kOddAddress = 0x105;
inline constexpr int kEndMissing = 0x106;
inline constexpr int kAddressMissing = 0x107;
inline constexpr int kThenExpected = 0x108;
inline constexpr int kDoExpected = 0x109;
inline constexpr int kForwardRef = 0x10A;
inline constexpr int kLabelTooLong = 0x10B;

inline constexpr int kSeverity = 0xF00;

// NEWERROR(var, code): keeps `var` unless `code` is more severe.
inline void NewError(int& var, int code) {
  if ((code & kSeverity) > var)
    var = code;
}

// True for statuses reported as errors (MINOR and above); WARNING-class
// statuses are warnings.
inline bool IsError(int code) {
  return (code & kSeverity) >= kMinor;
}

// The original's message for `code` without the "ERROR: " / "WARNING: "
// prefix (ERROR.CPP printError()); nullptr for unknown codes.
const char* Message(int code);

}  // namespace easym68k::asm_::status

#endif  // EASYM68K_ASM_ORIGINAL_STATUS_H_
