// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/symbol_table.h"

#include <gtest/gtest.h>

namespace easym68k::asm_ {
namespace {

// ---------------------------------------------------------------------------
// Basic define / lookup
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, DefineAndLookup) {
  SymbolTable st;
  EXPECT_TRUE(st.Define("FOO", 42, 1));
  SymbolInfo info;
  EXPECT_TRUE(st.Lookup("FOO", info));
  EXPECT_EQ(info.value, 42);
  EXPECT_TRUE(info.is_defined);
}

TEST(SymbolTableTest, CaseInsensitiveLookupByDefault) {
  SymbolTable st;
  st.Define("foo", 10, 1);
  SymbolInfo info;
  EXPECT_TRUE(st.Lookup("FOO", info));
  EXPECT_EQ(info.value, 10);
}

TEST(SymbolTableTest, CaseSensitiveLookup) {
  SymbolTable st;
  st.SetCaseSensitive(true);
  st.Define("foo", 10, 1);
  SymbolInfo info;
  EXPECT_FALSE(st.Lookup("FOO", info));
  EXPECT_TRUE(st.Lookup("foo", info));
}

TEST(SymbolTableTest, LookupMissingReturnsFalse) {
  SymbolTable st;
  SymbolInfo info;
  EXPECT_FALSE(st.Lookup("MISSING", info));
}

TEST(SymbolTableTest, ExistsAndIsDefined) {
  SymbolTable st;
  st.Define("BAR", 7, 1);
  EXPECT_TRUE(st.Exists("BAR"));
  EXPECT_TRUE(st.IsDefined("BAR"));
  EXPECT_FALSE(st.Exists("BAZ"));
  EXPECT_FALSE(st.IsDefined("BAZ"));
}

TEST(SymbolTableTest, GetValue) {
  SymbolTable st;
  st.Define("X", 99, 1);
  EXPECT_EQ(st.GetValue("X"), 99);
  EXPECT_EQ(st.GetValue("MISSING"), 0);
}

// ---------------------------------------------------------------------------
// EQU — permanent definition
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, DefineEquPermanent) {
  SymbolTable st;
  EXPECT_TRUE(st.DefineEqu("CONST", 100, 1));
  EXPECT_FALSE(st.DefineEqu("CONST", 200, 2));  // redefinition fails
  EXPECT_EQ(st.GetValue("CONST"), 100);
}

// ---------------------------------------------------------------------------
// SET — redefinable
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, DefineSetRedefinable) {
  SymbolTable st;
  EXPECT_TRUE(st.DefineSet("VAR", 1, 1));
  EXPECT_EQ(st.GetValue("VAR"), 1);
  EXPECT_TRUE(st.DefineSet("VAR", 2, 2));
  EXPECT_EQ(st.GetValue("VAR"), 2);
}

TEST(SymbolTableTest, SetCannotRedefineEqu) {
  SymbolTable st;
  st.DefineEqu("FIXED", 5, 1);
  EXPECT_FALSE(st.DefineSet("FIXED", 6, 2));
  EXPECT_EQ(st.GetValue("FIXED"), 5);
}

// ---------------------------------------------------------------------------
// Labels
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, DefineLabel) {
  SymbolTable st;
  EXPECT_TRUE(st.DefineLabel("start", 0x1000, 1));
  EXPECT_EQ(st.GetValue("start"), static_cast<int32_t>(0x1000));
}

TEST(SymbolTableTest, DefineLabelUpdatesParent) {
  SymbolTable st;
  st.DefineLabel("outer", 0x2000, 1);
  // Local label resolution: .loop resolves as outer.loop
  st.DefineLabel(".loop", 0x2010, 2);
  SymbolInfo info;
  EXPECT_TRUE(st.Lookup("outer.loop", info));
  EXPECT_EQ(info.value, static_cast<int32_t>(0x2010));
}

TEST(SymbolTableTest, DefineLabelDsFlag) {
  SymbolTable st;
  st.DefineLabel("buf", 0x3000, 1, /*is_ds=*/true);
  SymbolInfo info;
  EXPECT_TRUE(st.Lookup("buf", info));
  EXPECT_TRUE(HasFlag(info.flags, SymbolFlags::kDS));
}

// ---------------------------------------------------------------------------
// Register list
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, DefineRegisterList) {
  SymbolTable st;
  EXPECT_TRUE(st.DefineRegisterList("REGS", 0x00FF, 1));
  SymbolInfo info;
  EXPECT_TRUE(st.Lookup("REGS", info));
  EXPECT_TRUE(HasFlag(info.flags, SymbolFlags::kRegisterList));
  EXPECT_EQ(info.value, 0x00FF);
}

// ---------------------------------------------------------------------------
// Macro
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, DefineMacro) {
  SymbolTable st;
  EXPECT_TRUE(st.DefineMacro("MYMACRO", 1));
  SymbolInfo info;
  EXPECT_TRUE(st.Lookup("MYMACRO", info));
  EXPECT_TRUE(HasFlag(info.flags, SymbolFlags::kMacro));
}

// ---------------------------------------------------------------------------
// Forward references
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, AddForwardRef) {
  SymbolTable st;
  st.AddForwardRef("TARGET", 5, 0x1000, 2, false);
  const auto& refs = st.GetForwardRefs("TARGET");
  ASSERT_EQ(refs.size(), 1u);
  EXPECT_EQ(refs[0].address, 0x1000u);
  EXPECT_EQ(refs[0].size, 2);
  EXPECT_FALSE(refs[0].is_relative);
}

TEST(SymbolTableTest, ForwardRefCreatesUndefinedEntry) {
  SymbolTable st;
  st.AddForwardRef("FUTURE", 1, 0x200, 2, true);
  EXPECT_TRUE(st.Exists("FUTURE"));
  EXPECT_FALSE(st.IsDefined("FUTURE"));
}

TEST(SymbolTableTest, ResolveForwardRefsClearsRefs) {
  SymbolTable st;
  st.AddForwardRef("SYM", 1, 0x100, 2, false);
  st.ResolveForwardRefs("SYM");
  EXPECT_TRUE(st.GetForwardRefs("SYM").empty());
}

TEST(SymbolTableTest, GetForwardRefsOnMissingSymbolReturnsEmpty) {
  SymbolTable st;
  EXPECT_TRUE(st.GetForwardRefs("NOSYM").empty());
}

// ---------------------------------------------------------------------------
// Bulk queries
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, GetAllSymbols) {
  SymbolTable st;
  st.DefineEqu("B", 2, 1);
  st.DefineEqu("A", 1, 2);
  auto all = st.GetAllSymbols();
  ASSERT_EQ(all.size(), 2u);
  EXPECT_EQ(all[0].name, "A");
  EXPECT_EQ(all[1].name, "B");
}

TEST(SymbolTableTest, GetUndefinedSymbols) {
  SymbolTable st;
  st.AddForwardRef("UNDEF1", 1, 0, 2, false);
  st.AddForwardRef("UNDEF2", 1, 0, 2, false);
  st.DefineEqu("DEFINED", 1, 1);
  auto undef = st.GetUndefinedSymbols();
  EXPECT_EQ(undef.size(), 2u);
}

// ---------------------------------------------------------------------------
// MarkAsBackRef
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, MarkAsBackRef) {
  SymbolTable st;
  st.DefineEqu("SYM", 0, 1);
  st.MarkAsBackRef("SYM");
  SymbolInfo info;
  EXPECT_TRUE(st.Lookup("SYM", info));
  EXPECT_TRUE(HasFlag(info.flags, SymbolFlags::kBackRef));
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, ClearRemovesAll) {
  SymbolTable st;
  st.DefineEqu("A", 1, 1);
  st.Clear();
  EXPECT_FALSE(st.Exists("A"));
  EXPECT_TRUE(st.GetAllSymbols().empty());
}

// ---------------------------------------------------------------------------
// SetCurrentParentLabel
// ---------------------------------------------------------------------------

TEST(SymbolTableTest, SetCurrentParentLabel) {
  SymbolTable st;
  st.SetCurrentParentLabel("func");
  st.DefineLabel(".local", 0x5000, 1);
  SymbolInfo info;
  EXPECT_TRUE(st.Lookup("func.local", info));
  EXPECT_EQ(info.value, static_cast<int32_t>(0x5000));
}

}  // namespace
}  // namespace easym68k::asm_
