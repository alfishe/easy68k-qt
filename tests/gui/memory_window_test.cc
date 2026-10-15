// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// MemoryWindow (Tasks 13.6, 13.7): rows of hex/ASCII, scrolling, address
// field, cursor placement and editing as Memory1.cpp, Copy/Fill/Save, and
// Live copies while the program runs.

#include "memory_window.h"

#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

namespace easym68k::gui {
namespace {

class MemoryWindowTest : public ::testing::Test {
 protected:
  void SetUp() override {
    for (uint32_t i = 0; i < 0x40; i++)
      memory_.RawData()[0x1000 + i] = static_cast<uint8_t>(0x30 + i);
    MemoryWindow::Access access;
    access.readable = [this] { return readable_; };
    access.request_snapshot = [this](uint32_t a, uint32_t n) {
      requested_address_ = a;
      requested_size_ = n;
    };
    window_ = std::make_unique<MemoryWindow>(&memory_, access);
    window_->SetNotify([this](const QString& text) { notices_.append(text); });
    window_->resize(640, 450);
    window_->show();
  }

  sim::Memory memory_;
  bool readable_ = true;
  uint32_t requested_address_ = 0;
  uint32_t requested_size_ = 0;
  QStringList notices_;
  std::unique_ptr<MemoryWindow> window_;
};

TEST_F(MemoryWindowTest, RowsOfHexAndAscii) {
  window_->ShowAddress(0x1003);  // the row of the address
  EXPECT_EQ(window_->TopAddress(), 0x1000u);
  EXPECT_EQ(window_->AddressField()->text(), QStringLiteral("001000"));
  EXPECT_EQ(window_->RowText(0),
            QStringLiteral("00001000: 30 31 32 33 34 35 36 37 38 39 3A 3B 3C 3D 3E 3F "
                           "0123456789:;<=>?"));
  memory_.RawData()[0x1010] = 0x07;
  memory_.RawData()[0x1011] = 0xC8;
  EXPECT_TRUE(window_->RowText(1).endsWith(QStringLiteral("--BCDEFGHIJKLMNO")));
}

TEST_F(MemoryWindowTest, ScrollingStaysInMemory) {
  window_->ShowAddress(0x10);
  QTest::keyClick(window_->AddressField(), Qt::Key_Up);
  EXPECT_EQ(window_->TopAddress(), 0x0u);
  QTest::keyClick(window_->AddressField(), Qt::Key_Up);  // not below 0
  EXPECT_EQ(window_->TopAddress(), 0x0u);
  QTest::keyClick(window_->AddressField(), Qt::Key_PageDown);
  EXPECT_EQ(window_->TopAddress(), static_cast<uint32_t>(window_->Rows()) * 16);
  window_->ShowAddress(0xFFFFF0);
  QTest::keyClick(window_->AddressField(), Qt::Key_Down);
  EXPECT_EQ(window_->TopAddress(), 0xFFFFF0u);  // MEMSIZE - 16
  EXPECT_TRUE(window_->RowText(1).startsWith(QStringLiteral("01000000: xx xx")));
  // Typing an address moves to its row.
  window_->AddressField()->clear();
  QTest::keyClicks(window_->AddressField(), QStringLiteral("2a5"));
  EXPECT_EQ(window_->TopAddress(), 0x2A0u);
  EXPECT_EQ(window_->AddressField()->text(), QStringLiteral("2A5"));
}

TEST_F(MemoryWindowTest, HexAndAsciiEditing) {
  window_->ShowAddress(0x1000);
  int edits = 0;
  QObject::connect(window_.get(), &MemoryWindow::MemoryEdited, [&] { edits++; });
  window_->PlaceCursor(0, 9);  // the address column: moved to the first digit
  EXPECT_EQ(window_->CursorColumn(), MemoryWindow::kHexLeft);
  QTest::keyClicks(window_.get(), QStringLiteral("ab"));
  EXPECT_EQ(memory_.RawData()[0x1000], 0xAB);
  EXPECT_EQ(window_->CursorColumn(), 13);     // skips the space to the next byte
  QTest::keyClick(window_.get(), Qt::Key_G);  // not a hex digit: ignored
  EXPECT_EQ(memory_.RawData()[0x1001], 0x31);
  window_->PlaceCursor(0, 56);  // the last low nybble wraps to the next row
  QTest::keyClicks(window_.get(), QStringLiteral("0"));
  EXPECT_EQ(memory_.RawData()[0x100F], 0x30);
  EXPECT_EQ(window_->CursorRow(), 1);
  EXPECT_EQ(window_->CursorColumn(), MemoryWindow::kHexLeft);
  window_->PlaceCursor(2, MemoryWindow::kAscLeft + 3);
  QTest::keyClicks(window_.get(), QStringLiteral("Hi"));
  EXPECT_EQ(memory_.RawData()[0x1023], 'H');
  EXPECT_EQ(memory_.RawData()[0x1024], 'i');
  EXPECT_EQ(edits, 5);
  // Left arrow skips the spaces between hex bytes.
  window_->PlaceCursor(0, 13);
  QTest::keyClick(window_.get(), Qt::Key_Left);
  EXPECT_EQ(window_->CursorColumn(), 11);
  // No edits while the program runs.
  readable_ = false;
  window_->PlaceCursor(0, MemoryWindow::kAscLeft);
  QTest::keyClicks(window_.get(), QStringLiteral("Z"));
  EXPECT_NE(memory_.RawData()[0x1000], 'Z');
}

TEST_F(MemoryWindowTest, CopyHandlesOverlapAndChecksTheRange) {
  window_->FromField()->setText(QStringLiteral("001000"));
  window_->ToField()->setText(QStringLiteral("001002"));
  window_->BytesField()->setText(QStringLiteral("4"));
  window_->Copy();  // overlapping upward copy
  EXPECT_EQ(memory_.RawData()[0x1002], 0x30);
  EXPECT_EQ(memory_.RawData()[0x1005], 0x33);
  window_->FromField()->setText(QStringLiteral("FFFFF0"));
  window_->ToField()->setText(QStringLiteral("001000"));
  window_->BytesField()->setText(QStringLiteral("20"));
  window_->Copy();
  EXPECT_EQ(notices_.last(), QStringLiteral("Invalid copy range"));
  EXPECT_EQ(memory_.RawData()[0x1000], 0x30);  // nothing copied
  window_->BytesField()->setText(QStringLiteral("0"));
  window_->Copy();
  EXPECT_EQ(notices_.size(), 2);
}

TEST_F(MemoryWindowTest, FillRepeatsTheFourByteValue) {
  window_->FromField()->setText(QStringLiteral("002000"));
  window_->ToField()->setText(QStringLiteral("002005"));
  window_->BytesField()->setText(QStringLiteral("11223344"));
  window_->Fill();
  const uint8_t* m = memory_.RawData();
  EXPECT_EQ(m[0x2000], 0x11);
  EXPECT_EQ(m[0x2003], 0x44);
  EXPECT_EQ(m[0x2004], 0x11);
  EXPECT_EQ(m[0x2005], 0x22);
  EXPECT_EQ(m[0x2006], 0x00);
}

TEST_F(MemoryWindowTest, SaveWritesTheRange) {
  QTemporaryDir dir;
  window_->FromField()->setText(QStringLiteral("001000"));
  window_->ToField()->setText(QStringLiteral("001003"));
  const QString path = dir.filePath(QStringLiteral("mem.bin"));
  ASSERT_TRUE(window_->SaveTo(path));
  QFile f(path);
  ASSERT_TRUE(f.open(QIODevice::ReadOnly));
  EXPECT_EQ(f.readAll(), QByteArray("0123"));
  window_->ToField()->setText(QStringLiteral("000FFF"));  // To < From
  EXPECT_FALSE(window_->SaveTo(path));
  EXPECT_EQ(notices_.last(), QStringLiteral("Invalid memory range."));
}

TEST_F(MemoryWindowTest, LiveCopiesWhileRunning) {
  window_->ShowAddress(0x1000);
  readable_ = false;  // running
  window_->LiveBox()->setChecked(true);
  QTest::qWait(250);
  EXPECT_EQ(requested_address_, 0x1000u);
  EXPECT_EQ(requested_size_, static_cast<uint32_t>(window_->Rows()) * 16);
  QByteArray bytes(static_cast<int>(requested_size_), '\x41');
  window_->OnSnapshot(0x1000, bytes);
  EXPECT_TRUE(window_->RowText(0).endsWith(QStringLiteral("AAAAAAAAAAAAAAAA")));
}

}  // namespace
}  // namespace easym68k::gui
