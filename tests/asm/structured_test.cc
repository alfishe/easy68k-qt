// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Tests for Task 8.5.7: Structured Control Flow (IF/ELSE/ENDI, WHILE/ENDW,
// REPEAT/UNTIL, FOR/ENDF, DBLOOP/UNLESS)

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "easym68k/asm/assembler.h"

namespace easym68k::asm_ {
namespace {

// ─── helpers ────────────────────────────────────────────────────────────────

std::vector<uint8_t> Asm(const std::string& src) {
  Assembler a;
  auto result = a.Assemble(src);
  if (!result.success)
    for (const auto& e : result.errors)
      ADD_FAILURE() << "line " << e.line_number << ": " << e.message;
  return result.code;
}

void AsmExpectError(const std::string& src, const std::string& substr) {
  Assembler a;
  auto result = a.Assemble(src);
  bool found = false;
  for (const auto& e : result.errors)
    if (e.message.find(substr) != std::string::npos) {
      found = true;
      break;
    }
  if (result.success)
    ADD_FAILURE() << "Expected error '" << substr << "' but got success";
  else if (!found) {
    std::string all;
    for (const auto& e : result.errors)
      all += e.message + "; ";
    ADD_FAILURE() << "Expected error '" << substr << "', got: " << all;
  }
}

// Extract a big-endian 16-bit word from code at byte offset.
uint16_t Word(const std::vector<uint8_t>& code, size_t off) {
  return static_cast<uint16_t>((code[off] << 8) | code[off + 1]);
}

// Extract a big-endian signed 16-bit value.
int16_t SWord(const std::vector<uint8_t>& code, size_t off) {
  return static_cast<int16_t>(Word(code, off));
}

// ─── IF / ELSE / ENDI ───────────────────────────────────────────────────────

TEST(StructuredTest, IfSimpleCcBodyExecuted) {
  // IF <EQ> THEN … ENDI: body assembled (BNE skips over it)
  auto code =
      Asm("  ORG $1000\n"
          "  IF <EQ> THEN\n"
          "    NOP\n"
          "  ENDI\n");
  // Expected: BNE.W skip_label (4 bytes) + NOP (2 bytes) + skip_label
  // BNE.W: 0x6600 + 16-bit disp = 4 bytes total  NOP: 0x4E71
  ASSERT_EQ(code.size(), 6u);
  EXPECT_EQ(Word(code, 0), 0x6600u);  // BNE.W
  EXPECT_EQ(SWord(code, 2), 4);       // disp = $1006 - ($1000+2) = 4 (past the NOP)
  EXPECT_EQ(Word(code, 4), 0x4E71u);  // NOP
}

TEST(StructuredTest, IfElse) {
  // IF <EQ> THEN NOP ELSE MOVEQ #1,D0 ENDI
  auto code =
      Asm("  ORG $1000\n"
          "  IF <EQ> THEN\n"
          "    NOP\n"
          "  ELSE\n"
          "    MOVEQ #1,D0\n"
          "  ENDI\n");
  // BNE.W lbl_else (4) + NOP (2) + BRA.W lbl_end (4) + lbl_else: MOVEQ #1,D0 (2) + lbl_end:
  ASSERT_EQ(code.size(), 12u);
  EXPECT_EQ(Word(code, 0), 0x6600u);   // BNE.W
  EXPECT_EQ(SWord(code, 2), 8);        // disp = $100A - ($1000+2) = 8 (to else-body)
  EXPECT_EQ(Word(code, 4), 0x4E71u);   // NOP
  EXPECT_EQ(Word(code, 6), 0x6000u);   // BRA.W
  EXPECT_EQ(SWord(code, 8), 4);        // disp = $100C - ($1006+2) = 4 (to end)
  EXPECT_EQ(Word(code, 10), 0x7001u);  // MOVEQ #1,D0
}

TEST(StructuredTest, IfNestedTwoLevels) {
  // Nested IF: outer and inner each emit their own BNE
  auto code =
      Asm("  ORG $1000\n"
          "  IF <EQ> THEN\n"
          "    IF <GT> THEN\n"
          "      NOP\n"
          "    ENDI\n"
          "  ENDI\n");
  // outer BNE.W (4) + inner BNE.W (4) + NOP (2) = 10
  ASSERT_EQ(code.size(), 10u);
  EXPECT_EQ(Word(code, 0), 0x6600u);  // outer BNE.W
  EXPECT_EQ(Word(code, 4), 0x6F00u);  // inner BLE.W (for <GT>: BLE is skip-when-false)
}

TEST(StructuredTest, IfWithSizeSuffix) {
  // IF.B D0 <EQ> D1 THEN … ENDI — should generate CMP.B D1,D0 then BNE
  auto code =
      Asm("  ORG $1000\n"
          "  IF.B D0 <EQ> D1 THEN\n"
          "    NOP\n"
          "  ENDI\n");
  // CMP.B D1,D0 (2 bytes) + BNE.W (4 bytes) + NOP (2 bytes)
  ASSERT_EQ(code.size(), 8u);
  EXPECT_EQ(Word(code, 0), 0xB001u);  // CMP.B D1,D0
  EXPECT_EQ(Word(code, 2), 0x6600u);  // BNE.W
  EXPECT_EQ(SWord(code, 4), 4);       // disp = $1008 - ($1002+2) = 4
  EXPECT_EQ(Word(code, 6), 0x4E71u);  // NOP
}

TEST(StructuredTest, IfImmediateRight) {
  // IF D0 <GT> #5 THEN … ENDI → CMP.W #5,D0 + BLE.W + NOP
  auto code =
      Asm("  ORG $1000\n"
          "  IF D0 <GT> #5 THEN\n"
          "    NOP\n"
          "  ENDI\n");
  // CMP.W #5,D0 (4 bytes: opword + imm word) + BLE.W (4 bytes) + NOP (2 bytes)
  ASSERT_EQ(code.size(), 10u);
  EXPECT_EQ(Word(code, 0), 0xB07Cu);  // CMP.W #5,D0 (CMP with immediate EA)
  EXPECT_EQ(Word(code, 2), 5);        // immediate 5
  EXPECT_EQ(Word(code, 4), 0x6F00u);  // BLE.W
  EXPECT_EQ(SWord(code, 6), 4);       // disp = $100A - ($1004+2) = 4
  EXPECT_EQ(Word(code, 8), 0x4E71u);
}

// ─── WHILE / ENDW ───────────────────────────────────────────────────────────

TEST(StructuredTest, WhileSimple) {
  // WHILE <EQ> DO NOP ENDW
  // → top: BNE.W exit; NOP; BRA.W top; exit:
  auto code =
      Asm("  ORG $1000\n"
          "  WHILE <EQ> DO\n"
          "    NOP\n"
          "  ENDW\n");
  // top (0): BNE.W exit (4) + NOP (2) + BRA.W top (4) + exit:
  ASSERT_EQ(code.size(), 10u);
  EXPECT_EQ(Word(code, 0), 0x6600u);  // BNE.W
  EXPECT_EQ(SWord(code, 2), 8);       // disp = $100A - ($1000+2) = 8 (to exit)
  EXPECT_EQ(Word(code, 4), 0x4E71u);  // NOP
  EXPECT_EQ(Word(code, 6), 0x6000u);  // BRA.W
  EXPECT_EQ(SWord(code, 8), -8);      // disp = $1000 - ($1006+2) = -8 (back to top)
}

TEST(StructuredTest, WhileBackBranchDisplacement) {
  // Verify the BRA.W displacement goes back to loop top.
  // top at $1000 (0 bytes offset).
  // BNE.W skip: 4 bytes at $1000
  // NOP: 2 bytes at $1004
  // BRA.W top: at $1006, PC after fetch = $1008, target = $1000 → disp = -8
  auto code =
      Asm("  ORG $1000\n"
          "  WHILE <EQ> DO\n"
          "    NOP\n"
          "  ENDW\n");
  ASSERT_GE(code.size(), 10u);
  // BRA.W opword at offset 6; PC after opword fetch = offset 6+2=8; target offset=0 → disp=0-8=-8
  EXPECT_EQ(SWord(code, 8), -8);
}

TEST(StructuredTest, WhileInfinite) {
  // WHILE <T> DO (infinite): no condition branch, just top: NOP; BRA top
  auto code =
      Asm("  ORG $1000\n"
          "  WHILE <T> DO\n"
          "    NOP\n"
          "  ENDW\n");
  // top: NOP (2) + BRA.W top (4)
  ASSERT_EQ(code.size(), 6u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);  // NOP
  EXPECT_EQ(Word(code, 2), 0x6000u);  // BRA.W
  EXPECT_EQ(SWord(code, 4), -4);      // disp = $1000 - ($1002+2) = -4
}

TEST(StructuredTest, WhileNested) {
  // Nested WHILE uses independent label counters
  auto code =
      Asm("  ORG $1000\n"
          "  WHILE <EQ> DO\n"
          "    WHILE <GT> DO\n"
          "      NOP\n"
          "    ENDW\n"
          "  ENDW\n");
  // outer: top0; BNE exit0; inner: top1; BLE exit1; NOP; BRA top1; exit1; BRA top0; exit0
  // Should assemble without errors and produce consistent displacements.
  ASSERT_FALSE(code.empty());
  EXPECT_EQ(Word(code, 0), 0x6600u);  // outer BNE.W
}

// ─── REPEAT / UNTIL ─────────────────────────────────────────────────────────

TEST(StructuredTest, RepeatUntilSimple) {
  // REPEAT NOP UNTIL <EQ> DO → top: NOP; BNE.W top (branch-back when NOT EQ)
  auto code =
      Asm("  ORG $1000\n"
          "  REPEAT\n"
          "    NOP\n"
          "  UNTIL <EQ> DO\n");
  // top: NOP (2 bytes) + BNE.W disp (4 bytes) = 6
  ASSERT_EQ(code.size(), 6u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);  // NOP
  EXPECT_EQ(Word(code, 2),
            0x6600u);  // BNE.W (for <EQ> col-1 = BNE, branch when NOT equal = keep looping)
  EXPECT_EQ(SWord(code, 4), -4);  // disp = $1000 - ($1002+2) = -4
}

TEST(StructuredTest, RepeatUntilMultipleIterations) {
  // Body with two NOPs
  auto code =
      Asm("  ORG $1000\n"
          "  REPEAT\n"
          "    NOP\n"
          "    NOP\n"
          "  UNTIL <NE> DO\n");
  // top: NOP(2) + NOP(2) + BEQ.W disp(4) = 8
  // For <NE> col-1 = BEQ (branch when NOT <NE> = branch when equal = keep looping)
  ASSERT_EQ(code.size(), 8u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);
  EXPECT_EQ(Word(code, 2), 0x4E71u);
  EXPECT_EQ(Word(code, 4), 0x6700u);  // BEQ.W
  EXPECT_EQ(SWord(code, 6), -6);      // disp = $1000 - ($1004+2) = -6
}

// ─── FOR / ENDF ─────────────────────────────────────────────────────────────

TEST(StructuredTest, ForBasicTo) {
  // FOR D0 = #1 TO #10 DO NOP ENDF
  // Generated:  MOVE.W #1,D0; BRA check; body: NOP; ENDF: ADD.W #1,D0; check: CMP.W #10,D0; BLE
  // body
  auto code =
      Asm("  ORG $1000\n"
          "  FOR D0 = #1 TO #10 DO\n"
          "    NOP\n"
          "  ENDF\n");
  // MOVE.W #1,D0 = 4 bytes (MOVEQ #1,D0 = 2 bytes since value fits and it's .W... actually MOVE.W
  // #n,Dn is 4 bytes) Actually MOVE.W #1,D0: MOVEQ can handle this? No: FOR generates MOVE not
  // MOVEQ. MOVE.W #1,D0: 4 bytes (0x303C 0x0001) BRA.W check: 4 bytes NOP: 2 bytes ADD.W #1,D0: 4
  // bytes (ADDQ.W #1,D0: 2 bytes since ADDQ handles #1) CMP.W #10,D0: 4 bytes BLE.W body: 4 bytes
  // Total: 4+4+2+2+4+4 = 20 bytes (if ADD uses ADDQ)
  // or:   4+4+2+4+4+4 = 22 bytes (if ADD uses full form)
  ASSERT_FALSE(code.empty());
  // MOVE.W #1,D0 at start
  EXPECT_EQ(Word(code, 0), 0x303Cu);  // MOVE.W #n,D0
  EXPECT_EQ(Word(code, 2), 1u);       // immediate 1
  // BRA.W to check
  EXPECT_EQ(Word(code, 4), 0x6000u);
}

TEST(StructuredTest, ForDownTo) {
  // FOR D1 = #5 DOWNTO #1 DO NOP ENDF  → SUB instead of ADD, BGE instead of BLE
  auto code =
      Asm("  ORG $1000\n"
          "  FOR D1 = #5 DOWNTO #1 DO\n"
          "    NOP\n"
          "  ENDF\n");
  ASSERT_FALSE(code.empty());
  EXPECT_EQ(Word(code, 0), 0x323Cu);  // MOVE.W #n,D1
  EXPECT_EQ(Word(code, 2), 5u);
  // Somewhere in the output there should be a BGE.W instruction
  bool found_bge = false;
  for (size_t i = 0; i + 1 < code.size(); i += 2) {
    uint16_t w = Word(code, i);
    if ((w & 0xFF00u) == 0x6C00u) {  // BGE.W = 0x6C00
      found_bge = true;
      break;
    }
  }
  EXPECT_TRUE(found_bge) << "Expected BGE.W for DOWNTO loop";
}

TEST(StructuredTest, ForWithBy) {
  // FOR D0 = #0 TO #10 BY #2 DO NOP ENDF  → ADD.W #2,D0
  auto code =
      Asm("  ORG $1000\n"
          "  FOR D0 = #0 TO #10 BY #2 DO\n"
          "    NOP\n"
          "  ENDF\n");
  ASSERT_FALSE(code.empty());
  // Find ADD with immediate 2 (ADDQ #2 = 0x5440 for ADDQ.W #2,D0; or ADDI.W = 0x0640)
  bool found_add2 = false;
  for (size_t i = 0; i + 1 < code.size(); i += 2) {
    uint16_t w = Word(code, i);
    if (w == 0x5440u) {  // ADDQ.W #2,D0
      found_add2 = true;
      break;
    }
  }
  EXPECT_TRUE(found_add2) << "Expected ADDQ.W #2,D0 for BY #2";
}

TEST(StructuredTest, ForNested) {
  // Nested FOR loops assemble correctly (independent label counters)
  auto code =
      Asm("  ORG $1000\n"
          "  FOR D0 = #1 TO #3 DO\n"
          "    FOR D1 = #1 TO #3 DO\n"
          "      NOP\n"
          "    ENDF\n"
          "  ENDF\n");
  ASSERT_FALSE(code.empty());
}

TEST(StructuredTest, ForSkipsInitWhenSameReg) {
  // FOR D0 = D0 TO #5 DO NOP ENDF  → no MOVE (op1 == op2)
  auto code =
      Asm("  ORG $1000\n"
          "  FOR D0 = D0 TO #5 DO\n"
          "    NOP\n"
          "  ENDF\n");
  // First instruction should be BRA (no MOVE since D0=D0)
  ASSERT_FALSE(code.empty());
  EXPECT_EQ(Word(code, 0), 0x6000u);  // BRA.W (no MOVE init)
}

// ─── DBLOOP / UNLESS ────────────────────────────────────────────────────────

TEST(StructuredTest, DbloopUnlessSimple) {
  // DBLOOP D0 = #5 NOP UNLESS  → MOVE.W #5,D0; top: NOP; DBRA D0,top
  auto code =
      Asm("  ORG $1000\n"
          "  DBLOOP D0 = #5\n"
          "    NOP\n"
          "  UNLESS\n");
  // MOVE.W #5,D0 (4 bytes: MOVEQ or MOVE.W)
  // Since DBLOOP generates "MOVE\t#5,D0" (no size), parser picks MOVE.W by default → 4 bytes
  // Actually MOVE #5,D0 with immediate: MOVE.W #5,D0 = 4 bytes, or MOVEQ #5,D0 = 2 bytes if
  // the assembler optimizes. Let's check what MOVE without size gives.
  // Our assembler with MOVE (no size) → MOVE.W → 4 bytes for immediate (NOT MOVEQ because it's MOVE
  // not MOVEQ) Actually, MOVE.W #5,D0: base MOVE word=0x303C + imm word=0x0005 = 4 bytes top: NOP
  // (2 bytes) DBRA D0,top: 4 bytes Total: 4+2+4 = 10 bytes
  ASSERT_EQ(code.size(), 10u);
  EXPECT_EQ(Word(code, 0), 0x303Cu);  // MOVE.W #n,D0
  EXPECT_EQ(Word(code, 2), 5u);
  EXPECT_EQ(Word(code, 4), 0x4E71u);  // NOP
  EXPECT_EQ(Word(code, 6), 0x51C8u);  // DBRA D0 (DBRA = DBF = 0x51C8)
  EXPECT_EQ(SWord(code, 8), -4);      // disp = $1004 - ($1006+2) = -4
}

TEST(StructuredTest, DbloopUnlessFCondition) {
  // UNLESS <F> — same as plain UNLESS (DBRA)
  auto code =
      Asm("  ORG $1000\n"
          "  DBLOOP D1 = #3\n"
          "    NOP\n"
          "  UNLESS <F>\n");
  ASSERT_GE(code.size(), 4u);
  // Find DBRA D1 = 0x51C9
  bool found_dbra = false;
  for (size_t i = 0; i + 1 < code.size(); i += 2) {
    if (Word(code, i) == 0x51C9u) {
      found_dbra = true;
      break;
    }
  }
  EXPECT_TRUE(found_dbra) << "Expected DBRA D1 for UNLESS <F>";
}

TEST(StructuredTest, DbloopUnlessCc) {
  // DBLOOP D2 = #10 NOP UNLESS <EQ>  → DBNE D2,top (getBcc("<EQ>",1,0)="BNE")
  auto code =
      Asm("  ORG $1000\n"
          "  DBLOOP D2 = #10\n"
          "    NOP\n"
          "  UNLESS <EQ>\n");
  // Find DBNE D2 = 0x56CA
  bool found = false;
  for (size_t i = 0; i + 1 < code.size(); i += 2) {
    if (Word(code, i) == 0x56CAu) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found) << "Expected DBNE D2 for UNLESS <EQ>";
}

// ─── Displacement correctness ────────────────────────────────────────────────

TEST(StructuredTest, IfDisplacementWithLargerBody) {
  // IF <EQ> THEN; three NOPs; ENDI — skip displacement must be 6 (3×2 bytes)
  auto code =
      Asm("  ORG $1000\n"
          "  IF <EQ> THEN\n"
          "    NOP\n"
          "    NOP\n"
          "    NOP\n"
          "  ENDI\n");
  ASSERT_EQ(code.size(), 10u);
  EXPECT_EQ(Word(code, 0), 0x6600u);
  EXPECT_EQ(SWord(code, 2), 8);  // disp = $100A - ($1000+2) = 8 (skip 3 NOPs)
}

TEST(StructuredTest, WhileRegisterCondition) {
  // WHILE D0 <GT> D1 DO NOP ENDW
  // → top: CMP.W D1,D0 (2) + BLE.W exit (4) + NOP (2) + BRA.W top (4) + exit:
  auto code =
      Asm("  ORG $1000\n"
          "  WHILE D0 <GT> D1 DO\n"
          "    NOP\n"
          "  ENDW\n");
  ASSERT_EQ(code.size(), 12u);
  EXPECT_EQ(Word(code, 0), 0xB041u);  // CMP.W D1,D0
  EXPECT_EQ(Word(code, 2), 0x6F00u);  // BLE.W
  EXPECT_EQ(SWord(code, 4), 8);       // disp = $100C - ($1002+2) = 8 (to exit)
  EXPECT_EQ(Word(code, 6), 0x4E71u);  // NOP
  EXPECT_EQ(Word(code, 8), 0x6000u);  // BRA.W
  EXPECT_EQ(SWord(code, 10), -10);    // disp = $1000 - ($1008+2) = -10 (back to top)
}

// ─── Error cases ─────────────────────────────────────────────────────────────

TEST(StructuredTest, EndiWithoutIf) {
  AsmExpectError(
      "  ORG $1000\n"
      "  ENDI\n",
      "ENDI");
}

TEST(StructuredTest, EndwWithoutWhile) {
  AsmExpectError(
      "  ORG $1000\n"
      "  ENDW\n",
      "ENDW");
}

TEST(StructuredTest, EndfWithoutFor) {
  AsmExpectError(
      "  ORG $1000\n"
      "  ENDF\n",
      "ENDF");
}

TEST(StructuredTest, UntilWithoutRepeat) {
  AsmExpectError(
      "  ORG $1000\n"
      "  UNTIL <EQ> DO\n",
      "UNTIL");
}

TEST(StructuredTest, UnlessWithoutDbloop) {
  AsmExpectError(
      "  ORG $1000\n"
      "  UNLESS\n",
      "UNLESS");
}

TEST(StructuredTest, ElseWithoutIf) {
  AsmExpectError(
      "  ORG $1000\n"
      "  ELSE\n",
      "ELSE");
}

TEST(StructuredTest, WhileMissingDo) {
  AsmExpectError(
      "  ORG $1000\n"
      "  WHILE <EQ>\n"
      "    NOP\n"
      "  ENDW\n",
      "DO");
}

TEST(StructuredTest, ForMissingDo) {
  AsmExpectError(
      "  ORG $1000\n"
      "  FOR D0 = #1 TO #5\n"
      "    NOP\n"
      "  ENDF\n",
      "DO");
}

TEST(StructuredTest, MismatchedNesting) {
  // Open a WHILE but try to close with ENDI
  AsmExpectError(
      "  ORG $1000\n"
      "  WHILE <EQ> DO\n"
      "    NOP\n"
      "  ENDI\n",
      "ENDI");
}

// ─── Two-pass consistency ────────────────────────────────────────────────────

TEST(StructuredTest, TwoPassConsistentLabels) {
  // Assemble twice: output must be identical (labels resolved in both passes)
  const std::string src =
      "  ORG $1000\n"
      "  IF <EQ> THEN\n"
      "    NOP\n"
      "  ELSE\n"
      "    NOP\n"
      "  ENDI\n";
  Assembler a1, a2;
  auto r1 = a1.Assemble(src);
  auto r2 = a2.Assemble(src);
  EXPECT_TRUE(r1.success);
  EXPECT_TRUE(r2.success);
  EXPECT_EQ(r1.code, r2.code);
}

// ─── Labels on structured directives ─────────────────────────────────────────

TEST(StructuredTest, LabelOnWhile) {
  // A label before WHILE should be accessible for jumps
  auto code =
      Asm("  ORG $1000\n"
          "myloop WHILE <EQ> DO\n"
          "    NOP\n"
          "  ENDW\n");
  // myloop: defined at $1000 (before the BNE), same as loop top
  // code structure same as basic WHILE test
  ASSERT_FALSE(code.empty());
  EXPECT_EQ(Word(code, 0), 0x6600u);  // BNE.W
}

// ─── WHILE with short branch suffix ──────────────────────────────────────────

TEST(StructuredTest, WhileShortBranch) {
  // WHILE <EQ> DO.S uses short branch (BNE.S = 2 bytes)
  auto code =
      Asm("  ORG $1000\n"
          "  WHILE <EQ> DO.S\n"
          "    NOP\n"
          "  ENDW\n");
  // BNE.S (2 bytes) + NOP (2 bytes) + BRA.W top (4 bytes) = 8 bytes
  // BNE.S: 0x6600 with byte displacement (0x66xx where xx != 0)
  ASSERT_EQ(code.size(), 8u);
  uint8_t ophi = code[0];
  EXPECT_EQ(ophi, 0x66u);  // BNE
  uint8_t disp = code[1];
  EXPECT_NE(disp, 0x00u);             // not word form (which uses 0x00 + 16-bit)
  EXPECT_EQ(Word(code, 2), 0x4E71u);  // NOP
}

}  // namespace
}  // namespace easym68k::asm_
