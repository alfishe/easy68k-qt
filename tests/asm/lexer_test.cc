// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/lexer.h"

#include <gtest/gtest.h>

namespace easym68k::asm_ {
namespace {

// ---------------------------------------------------------------------------
// Helper: tokenize a single line and return the first non-comment,
// non-newline, non-EOF token.
// ---------------------------------------------------------------------------
Token FirstToken(const std::string& text) {
  Lexer lex;
  lex.SetSource(text);
  return lex.NextToken();
}

// ---------------------------------------------------------------------------
// Token type
// ---------------------------------------------------------------------------

TEST(LexerTest, EofOnEmptySource) {
  Lexer lex;
  lex.SetSource("");
  EXPECT_EQ(lex.NextToken().type, TokenType::kEof);
}

TEST(LexerTest, NewlineToken) {
  Token t = FirstToken("\n");
  EXPECT_EQ(t.type, TokenType::kNewline);
}

// ---------------------------------------------------------------------------
// Comments
// ---------------------------------------------------------------------------

TEST(LexerTest, SemicolonComment) {
  Token t = FirstToken("; this is a comment");
  EXPECT_EQ(t.type, TokenType::kComment);
}

TEST(LexerTest, StarCommentAtColumn1) {
  Token t = FirstToken("* this is a comment");
  EXPECT_EQ(t.type, TokenType::kComment);
}

TEST(LexerTest, StarNotCommentMidLine) {
  // * not at column 1 is the multiplication operator
  Token t = FirstToken("1*2");
  EXPECT_EQ(t.type, TokenType::kNumber);
  Lexer lex;
  lex.SetSource("1*2");
  lex.NextToken();  // skip 1
  Token op = lex.NextToken();
  EXPECT_EQ(op.type, TokenType::kOperator);
  EXPECT_EQ(op.text, "*");
}

// ---------------------------------------------------------------------------
// Numbers
// ---------------------------------------------------------------------------

TEST(LexerTest, DecimalNumber) {
  Token t = FirstToken("42");
  EXPECT_EQ(t.type, TokenType::kNumber);
  EXPECT_EQ(t.int_value, 42);
  EXPECT_EQ(t.text, "42");
}

TEST(LexerTest, HexNumber) {
  Token t = FirstToken("$FF");
  EXPECT_EQ(t.type, TokenType::kNumber);
  EXPECT_EQ(t.int_value, 0xFF);
}

TEST(LexerTest, HexNumberLower) {
  Token t = FirstToken("$deadbeef");
  EXPECT_EQ(t.type, TokenType::kNumber);
  EXPECT_EQ(t.int_value, static_cast<int32_t>(0xdeadbeef));
}

TEST(LexerTest, BinaryNumber) {
  Token t = FirstToken("%1010");
  EXPECT_EQ(t.type, TokenType::kNumber);
  EXPECT_EQ(t.int_value, 0b1010);
}

TEST(LexerTest, OctalNumber) {
  Token t = FirstToken("@10");  // octal 10 = decimal 8
  EXPECT_EQ(t.type, TokenType::kNumber);
  EXPECT_EQ(t.int_value, 8);
}

TEST(LexerTest, HexMissingDigitIsError) {
  Token t = FirstToken("$");
  EXPECT_EQ(t.type, TokenType::kError);
}

TEST(LexerTest, BinaryMissingDigitIsError) {
  Token t = FirstToken("%");
  EXPECT_EQ(t.type, TokenType::kError);
}

TEST(LexerTest, OctalMissingDigitIsError) {
  Token t = FirstToken("@");
  EXPECT_EQ(t.type, TokenType::kError);
}

// ---------------------------------------------------------------------------
// Strings
// ---------------------------------------------------------------------------

TEST(LexerTest, SimpleString) {
  Token t = FirstToken("'hello'");
  EXPECT_EQ(t.type, TokenType::kString);
  EXPECT_EQ(t.text, "hello");
}

TEST(LexerTest, EscapedQuoteInString) {
  Token t = FirstToken("'it''s'");
  EXPECT_EQ(t.type, TokenType::kString);
  EXPECT_EQ(t.text, "it's");
}

TEST(LexerTest, SingleCharStringPackedValue) {
  Token t = FirstToken("'A'");
  EXPECT_EQ(t.type, TokenType::kString);
  EXPECT_EQ(t.int_value, 'A');
}

TEST(LexerTest, TwoCharStringPackedValue) {
  Token t = FirstToken("'AB'");
  EXPECT_EQ(t.type, TokenType::kString);
  EXPECT_EQ(t.int_value, ('A' << 8) | 'B');
}

// ---------------------------------------------------------------------------
// Labels
// ---------------------------------------------------------------------------

TEST(LexerTest, LabelWithColon) {
  Token t = FirstToken("start:");
  EXPECT_EQ(t.type, TokenType::kLabel);
  EXPECT_EQ(t.text, "start");
}

// ---------------------------------------------------------------------------
// Registers
// ---------------------------------------------------------------------------

TEST(LexerTest, DataRegisters) {
  for (int i = 0; i <= 7; ++i) {
    Token t = FirstToken("D" + std::to_string(i));
    EXPECT_EQ(t.type, TokenType::kRegister) << "D" << i;
    EXPECT_EQ(t.reg_type, RegisterType::kData) << "D" << i;
    EXPECT_EQ(t.reg_num, i) << "D" << i;
  }
}

TEST(LexerTest, AddressRegisters) {
  for (int i = 0; i <= 7; ++i) {
    Token t = FirstToken("A" + std::to_string(i));
    EXPECT_EQ(t.type, TokenType::kRegister) << "A" << i;
    EXPECT_EQ(t.reg_type, RegisterType::kAddress) << "A" << i;
    EXPECT_EQ(t.reg_num, i) << "A" << i;
  }
}

TEST(LexerTest, SpecialRegisters) {
  EXPECT_EQ(FirstToken("SP").reg_type, RegisterType::kSP);
  EXPECT_EQ(FirstToken("PC").reg_type, RegisterType::kPC);
  EXPECT_EQ(FirstToken("SR").reg_type, RegisterType::kSR);
  EXPECT_EQ(FirstToken("CCR").reg_type, RegisterType::kCCR);
  EXPECT_EQ(FirstToken("USP").reg_type, RegisterType::kUSP);
  EXPECT_EQ(FirstToken("VBR").reg_type, RegisterType::kVBR);
  EXPECT_EQ(FirstToken("SFC").reg_type, RegisterType::kSFC);
  EXPECT_EQ(FirstToken("DFC").reg_type, RegisterType::kDFC);
}

TEST(LexerTest, RegisterCaseInsensitive) {
  EXPECT_EQ(FirstToken("d0").reg_type, RegisterType::kData);
  EXPECT_EQ(FirstToken("a7").reg_type, RegisterType::kAddress);
  EXPECT_EQ(FirstToken("sp").reg_type, RegisterType::kSP);
}

// ---------------------------------------------------------------------------
// Opcodes — sample from each group
// ---------------------------------------------------------------------------

TEST(LexerTest, MoveOpcode) {
  Token t = FirstToken("MOVE");
  EXPECT_EQ(t.type, TokenType::kOpcode);
  EXPECT_EQ(t.text, "MOVE");
}

TEST(LexerTest, OpcodeWithSizeSpec) {
  Token t = FirstToken("MOVE.W");
  EXPECT_EQ(t.type, TokenType::kOpcode);
  EXPECT_EQ(t.size, SizeSpec::kWord);
}

TEST(LexerTest, AddOpcodes) {
  for (const char* m : {"ADD", "ADDA", "ADDI", "ADDQ", "ADDX"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, SubOpcodes) {
  for (const char* m : {"SUB", "SUBA", "SUBI", "SUBQ", "SUBX"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, MulDivOpcodes) {
  for (const char* m : {"MULS", "MULU", "DIVS", "DIVU"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, LogicOpcodes) {
  for (const char* m : {"AND", "ANDI", "OR", "ORI", "EOR", "EORI", "NOT"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, ShiftOpcodes) {
  for (const char* m : {"ASL", "ASR", "LSL", "LSR", "ROL", "ROR", "ROXL", "ROXR"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, BitOpcodes) {
  for (const char* m : {"BTST", "BSET", "BCLR", "BCHG"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, BcdOpcodes) {
  for (const char* m : {"ABCD", "SBCD", "NBCD"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, BranchOpcodes) {
  for (const char* m : {"BRA", "BSR", "BCC", "BCS", "BEQ", "BNE", "BGE", "BGT", "BHI", "BLE", "BLS",
                        "BLT", "BMI", "BPL", "BVC", "BVS"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, DbccOpcodes) {
  for (const char* m : {"DBCC", "DBCS", "DBEQ", "DBNE", "DBF", "DBT", "DBRA"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, SccOpcodes) {
  for (const char* m : {"SCC", "SCS", "SEQ", "SNE", "SGE", "SGT", "SHI", "SLE", "SLS", "SLT", "SMI",
                        "SPL", "SVC", "SVS", "SF", "ST"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, JumpOpcodes) {
  for (const char* m : {"JMP", "JSR", "RTS", "RTR", "RTE"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, MiscOpcodes) {
  for (const char* m : {"NOP", "RESET", "STOP", "ILLEGAL", "TRAP", "TRAPV", "TST", "TAS", "CHK",
                        "CLR", "NEG", "NEGX", "EXT", "SWAP", "LINK", "UNLK", "LEA", "PEA", "EXG"}) {
    EXPECT_EQ(FirstToken(m).type, TokenType::kOpcode) << m;
  }
}

TEST(LexerTest, OpcodesCaseInsensitive) {
  EXPECT_EQ(FirstToken("move").type, TokenType::kOpcode);
  EXPECT_EQ(FirstToken("Move").type, TokenType::kOpcode);
  EXPECT_EQ(FirstToken("MOVEQ").type, TokenType::kOpcode);
}

// ---------------------------------------------------------------------------
// Directives — EASy68K-specific coverage
// ---------------------------------------------------------------------------

// Data definition
TEST(LexerTest, DcDirective) {
  EXPECT_EQ(FirstToken("DC").type, TokenType::kDirective);
}

TEST(LexerTest, DcWithSize) {
  Token t = FirstToken("DC.W");
  EXPECT_EQ(t.type, TokenType::kDirective);
  EXPECT_EQ(t.size, SizeSpec::kWord);
}

TEST(LexerTest, DcbDirective) {
  EXPECT_EQ(FirstToken("DCB").type, TokenType::kDirective);
}

TEST(LexerTest, DsDirective) {
  EXPECT_EQ(FirstToken("DS").type, TokenType::kDirective);
}

// Symbol definition
TEST(LexerTest, EquDirective) {
  EXPECT_EQ(FirstToken("EQU").type, TokenType::kDirective);
}

TEST(LexerTest, SetDirective) {
  EXPECT_EQ(FirstToken("SET").type, TokenType::kDirective);
}

// Origin / section
TEST(LexerTest, OrgDirective) {
  EXPECT_EQ(FirstToken("ORG").type, TokenType::kDirective);
}

TEST(LexerTest, SectionDirective) {
  EXPECT_EQ(FirstToken("SECTION").type, TokenType::kDirective);
}

TEST(LexerTest, OffsetDirective) {
  EXPECT_EQ(FirstToken("OFFSET").type, TokenType::kDirective);
}

TEST(LexerTest, MemoryDirective) {
  EXPECT_EQ(FirstToken("MEMORY").type, TokenType::kDirective);
}

// End
TEST(LexerTest, EndDirective) {
  EXPECT_EQ(FirstToken("END").type, TokenType::kDirective);
}

TEST(LexerTest, SimhaltDirective) {
  EXPECT_EQ(FirstToken("SIMHALT").type, TokenType::kDirective);
}

// File inclusion
TEST(LexerTest, IncludeDirective) {
  EXPECT_EQ(FirstToken("INCLUDE").type, TokenType::kDirective);
}

TEST(LexerTest, IncbinDirective) {
  EXPECT_EQ(FirstToken("INCBIN").type, TokenType::kDirective);
}

// Listing control
TEST(LexerTest, ListDirective) {
  EXPECT_EQ(FirstToken("LIST").type, TokenType::kDirective);
}

TEST(LexerTest, NolistDirective) {
  EXPECT_EQ(FirstToken("NOLIST").type, TokenType::kDirective);
}

TEST(LexerTest, PageDirective) {
  EXPECT_EQ(FirstToken("PAGE").type, TokenType::kDirective);
}

TEST(LexerTest, OptDirective) {
  EXPECT_EQ(FirstToken("OPT").type, TokenType::kDirective);
}

// Macro
TEST(LexerTest, MacroDirective) {
  EXPECT_EQ(FirstToken("MACRO").type, TokenType::kDirective);
}

TEST(LexerTest, EndmDirective) {
  EXPECT_EQ(FirstToken("ENDM").type, TokenType::kDirective);
}

TEST(LexerTest, MendDirective) {
  EXPECT_EQ(FirstToken("MEND").type, TokenType::kDirective);
}

TEST(LexerTest, MexitDirective) {
  EXPECT_EQ(FirstToken("MEXIT").type, TokenType::kDirective);
}

// Conditionals
TEST(LexerTest, IfDirective) {
  EXPECT_EQ(FirstToken("IF").type, TokenType::kDirective);
}

TEST(LexerTest, ElseDirective) {
  EXPECT_EQ(FirstToken("ELSE").type, TokenType::kDirective);
}

TEST(LexerTest, EndiDirective) {
  EXPECT_EQ(FirstToken("ENDI").type, TokenType::kDirective);
}

// Structured control flow
TEST(LexerTest, WhileDirective) {
  EXPECT_EQ(FirstToken("WHILE").type, TokenType::kDirective);
}

TEST(LexerTest, EndwDirective) {
  EXPECT_EQ(FirstToken("ENDW").type, TokenType::kDirective);
}

TEST(LexerTest, RepeatDirective) {
  EXPECT_EQ(FirstToken("REPEAT").type, TokenType::kDirective);
}

TEST(LexerTest, UntilDirective) {
  EXPECT_EQ(FirstToken("UNTIL").type, TokenType::kDirective);
}

TEST(LexerTest, ForDirective) {
  EXPECT_EQ(FirstToken("FOR").type, TokenType::kDirective);
}

TEST(LexerTest, EndfDirective) {
  EXPECT_EQ(FirstToken("ENDF").type, TokenType::kDirective);
}

TEST(LexerTest, ThenDirective) {
  EXPECT_EQ(FirstToken("THEN").type, TokenType::kDirective);
}

TEST(LexerTest, DbloopDirective) {
  EXPECT_EQ(FirstToken("DBLOOP").type, TokenType::kDirective);
}

TEST(LexerTest, UnlessDirective) {
  EXPECT_EQ(FirstToken("UNLESS").type, TokenType::kDirective);
}

TEST(LexerTest, SwitchDirective) {
  EXPECT_EQ(FirstToken("SWITCH").type, TokenType::kDirective);
}

TEST(LexerTest, EndswDirective) {
  EXPECT_EQ(FirstToken("ENDSW").type, TokenType::kDirective);
}

// External symbols
TEST(LexerTest, XrefDirective) {
  EXPECT_EQ(FirstToken("XREF").type, TokenType::kDirective);
}

TEST(LexerTest, XdefDirective) {
  EXPECT_EQ(FirstToken("XDEF").type, TokenType::kDirective);
}

// Execution-plan-listed (may not exist in original but required by plan)
TEST(LexerTest, LoadDirective) {
  EXPECT_EQ(FirstToken("LOAD").type, TokenType::kDirective);
}

TEST(LexerTest, CommentDirective) {
  // COMMENT as a directive keyword (distinct from ; or * comment syntax)
  // Must appear somewhere other than column 1 to avoid being scanned as a comment.
  Lexer lex;
  lex.SetSource("  COMMENT");
  Token t = lex.NextToken();
  EXPECT_EQ(t.type, TokenType::kDirective);
}

TEST(LexerTest, DirectivesCaseInsensitive) {
  EXPECT_EQ(FirstToken("org").type, TokenType::kDirective);
  EXPECT_EQ(FirstToken("Equ").type, TokenType::kDirective);
  EXPECT_EQ(FirstToken("dc").type, TokenType::kDirective);
}

// ---------------------------------------------------------------------------
// Punctuation / operators
// ---------------------------------------------------------------------------

TEST(LexerTest, CommaToken) {
  Token t = FirstToken(",");
  EXPECT_EQ(t.type, TokenType::kComma);
}

TEST(LexerTest, LParenToken) {
  EXPECT_EQ(FirstToken("(").type, TokenType::kLParen);
}

TEST(LexerTest, RParenToken) {
  EXPECT_EQ(FirstToken(")").type, TokenType::kRParen);
}

TEST(LexerTest, HashToken) {
  EXPECT_EQ(FirstToken("#").type, TokenType::kHash);
}

TEST(LexerTest, PlusToken) {
  EXPECT_EQ(FirstToken("+").type, TokenType::kPlus);
}

TEST(LexerTest, MinusToken) {
  EXPECT_EQ(FirstToken("-").type, TokenType::kMinus);
}

TEST(LexerTest, DotToken) {
  EXPECT_EQ(FirstToken(".").type, TokenType::kDot);
}

TEST(LexerTest, OperatorTokens) {
  EXPECT_EQ(FirstToken("/").type, TokenType::kOperator);
  EXPECT_EQ(FirstToken("&").type, TokenType::kOperator);
  EXPECT_EQ(FirstToken("|").type, TokenType::kOperator);
  EXPECT_EQ(FirstToken("!").type, TokenType::kOperator);  // EASy68K OR
  EXPECT_EQ(FirstToken("^").type, TokenType::kOperator);
  EXPECT_EQ(FirstToken("~").type, TokenType::kOperator);
  EXPECT_EQ(FirstToken("\\").type, TokenType::kOperator);  // modulo
}

TEST(LexerTest, ShiftOperators) {
  EXPECT_EQ(FirstToken("<<").type, TokenType::kOperator);
  EXPECT_EQ(FirstToken(">>").type, TokenType::kOperator);
}

TEST(LexerTest, SingleLessIsError) {
  Token t = FirstToken("<");
  EXPECT_EQ(t.type, TokenType::kError);
}

TEST(LexerTest, SingleGreaterIsError) {
  Token t = FirstToken(">");
  EXPECT_EQ(t.type, TokenType::kError);
}

// ---------------------------------------------------------------------------
// Symbol
// ---------------------------------------------------------------------------

TEST(LexerTest, UnknownIdentifierIsSymbol) {
  Token t = FirstToken("my_symbol");
  EXPECT_EQ(t.type, TokenType::kSymbol);
  EXPECT_EQ(t.text, "my_symbol");
}

TEST(LexerTest, SymbolWithSizeSuffix) {
  Token t = FirstToken("label.L");
  EXPECT_EQ(t.type, TokenType::kSymbol);
  EXPECT_EQ(t.size, SizeSpec::kLong);
}

// ---------------------------------------------------------------------------
// PeekToken / NextToken interaction
// ---------------------------------------------------------------------------

TEST(LexerTest, PeekDoesNotConsume) {
  Lexer lex;
  lex.SetSource("MOVE.W");
  Token peeked = lex.PeekToken();
  Token next = lex.NextToken();
  EXPECT_EQ(peeked.type, next.type);
  EXPECT_EQ(peeked.text, next.text);
  EXPECT_EQ(peeked.size, next.size);
}

TEST(LexerTest, DoublePeekReturnsSameToken) {
  Lexer lex;
  lex.SetSource("D0");
  Token first = lex.PeekToken();
  Token second = lex.PeekToken();
  EXPECT_EQ(first.type, second.type);
  EXPECT_EQ(first.reg_num, second.reg_num);
}

// ---------------------------------------------------------------------------
// TokenizeLine
// ---------------------------------------------------------------------------

TEST(LexerTest, TokenizeLineBasic) {
  Lexer lex;
  auto tokens = lex.TokenizeLine("  MOVE.W D0,D1");
  // Expect: OPCODE REGISTER COMMA REGISTER EOF/NEWLINE
  ASSERT_GE(tokens.size(), 4u);
  EXPECT_EQ(tokens[0].type, TokenType::kOpcode);
  EXPECT_EQ(tokens[0].size, SizeSpec::kWord);
  EXPECT_EQ(tokens[1].type, TokenType::kRegister);
  EXPECT_EQ(tokens[2].type, TokenType::kComma);
  EXPECT_EQ(tokens[3].type, TokenType::kRegister);
}

TEST(LexerTest, TokenizeLineWithLabel) {
  Lexer lex;
  auto tokens = lex.TokenizeLine("loop: BRA loop");
  ASSERT_GE(tokens.size(), 3u);
  EXPECT_EQ(tokens[0].type, TokenType::kLabel);
  EXPECT_EQ(tokens[1].type, TokenType::kOpcode);
  EXPECT_EQ(tokens[2].type, TokenType::kSymbol);
}

TEST(LexerTest, TokenizeLinePreservesLineNumber) {
  Lexer lex;
  auto tokens = lex.TokenizeLine("NOP", 42);
  EXPECT_EQ(tokens[0].line, 42);
}

// ---------------------------------------------------------------------------
// Line / column tracking
// ---------------------------------------------------------------------------

TEST(LexerTest, LineAndColumnTracking) {
  Lexer lex;
  lex.SetSource("MOVE\nADD");
  Token t1 = lex.NextToken();
  EXPECT_EQ(t1.line, 1);
  lex.NextToken();  // newline
  Token t2 = lex.NextToken();
  EXPECT_EQ(t2.line, 2);
}

// ---------------------------------------------------------------------------
// Error state
// ---------------------------------------------------------------------------

TEST(LexerTest, NoErrorOnValidInput) {
  Lexer lex;
  lex.SetSource("MOVE.W D0,D1");
  while (lex.NextToken().type != TokenType::kEof) {
  }
  EXPECT_FALSE(lex.HasError());
}

TEST(LexerTest, ErrorOnInvalidChar) {
  Token t = FirstToken("?");
  EXPECT_EQ(t.type, TokenType::kError);
}

TEST(LexerTest, TokenTypeToStringCoversAll) {
  EXPECT_STRNE(TokenTypeToString(TokenType::kLabel), "UNKNOWN");
  EXPECT_STRNE(TokenTypeToString(TokenType::kOpcode), "UNKNOWN");
  EXPECT_STRNE(TokenTypeToString(TokenType::kDirective), "UNKNOWN");
  EXPECT_STRNE(TokenTypeToString(TokenType::kEof), "UNKNOWN");
  EXPECT_STRNE(TokenTypeToString(TokenType::kError), "UNKNOWN");
}

}  // namespace
}  // namespace easym68k::asm_
