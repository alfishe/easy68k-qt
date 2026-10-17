// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QByteArray>
#include <QFile>
#include <QKeyEvent>
#include <QTemporaryDir>
#include <QTest>

#include "easybin/mainwindow.h"
#include "gtest/gtest.h"
#include "mainwindow.h"

namespace easym68k::easybin {
namespace {

QByteArray ReadAllBytes(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return file.readAll();
}

TEST(EasyBinTest, InitialStateAndNewEnvironment) {
  MainWindow win;
  win.SetNotify([](const QString&) {});
  EXPECT_EQ(win.windowTitle(), QStringLiteral("EASyBIN v2.5.0"));
  EXPECT_EQ(win.GetMemory().RawData()[0], 0xFF);
  EXPECT_EQ(win.GetMemory().RawData()[0x1000], 0xFF);
  EXPECT_EQ(win.StartAddressField()->text(), QStringLiteral("00000000"));
  EXPECT_EQ(win.S68EndAddressLabel()->text(), QStringLiteral("000000"));
  EXPECT_EQ(win.S68LengthLabel()->text(), QStringLiteral("000000"));
  EXPECT_EQ(win.OutputFirstAddressField()->text(), QStringLiteral("000000"));
  EXPECT_EQ(win.OutputLengthField()->text(), QStringLiteral("000000"));
  EXPECT_EQ(win.FillByteField()->text(), QStringLiteral("FF"));
  EXPECT_EQ(win.Split(), 0);

  win.GetMemory().RawData()[0x100] = 0x12;
  win.StartAddressField()->setText(QStringLiteral("00001000"));
  win.SetSplitIndex(2);
  EXPECT_EQ(win.Split(), 4);

  win.NewEnvironment(/*confirm=*/false);
  EXPECT_EQ(win.windowTitle(), QStringLiteral("EASyBIN v2.5.0"));
  EXPECT_EQ(win.GetMemory().RawData()[0x100], 0xFF);
  EXPECT_EQ(win.StartAddressField()->text(), QStringLiteral("00000000"));
  EXPECT_EQ(win.Split(), 0);
}

TEST(EasyBinTest, OpenSRecordAndSplitColoring) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const QString s68_path = temp_dir.filePath(QStringLiteral("test.S68"));
  {
    QFile f(s68_path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    // S0: "TEST", S1 at $1000 with 8 bytes: 11 22 33 44 55 66 77 88, S8 at $1000
    f.write("S007000054455354B8\n");
    f.write("S10B10001122334455667788A0\n");
    f.write("S804001000EB\n");
  }

  MainWindow win;
  win.SetNotify([](const QString&) {});
  win.show();
  ASSERT_TRUE(win.OpenSRecordFile(s68_path));
  ASSERT_FALSE(win.Notifications().isEmpty());
  EXPECT_TRUE(win.Notifications().last().contains(QStringLiteral("S Record loaded successfully.")));
  EXPECT_TRUE(win.Notifications().last().contains(QStringLiteral("S0 = TEST")));

  EXPECT_EQ(win.StartAddressField()->text(), QStringLiteral("00001000"));
  EXPECT_EQ(win.S68EndAddressLabel()->text(), QStringLiteral("001007"));
  EXPECT_EQ(win.S68LengthLabel()->text(), QStringLiteral("000008"));
  EXPECT_EQ(win.AddressField()->text(), QStringLiteral("001000"));
  EXPECT_EQ(win.OutputFirstAddressField()->text(), QStringLiteral("001000"));
  EXPECT_EQ(win.OutputLengthField()->text(), QStringLiteral("000008"));

  // Split 0 (1 file): all 8 bytes in [0x1000..0x1007] are black; 0x1008 is gray.
  EXPECT_EQ(win.ByteColor(0x1000), QColor(Qt::black));
  EXPECT_EQ(win.ByteColor(0x1007), QColor(Qt::black));
  EXPECT_EQ(win.ByteColor(0x1008), QColor(Qt::gray));
  EXPECT_TRUE(win.SplitLabel(0)->isVisible());
  EXPECT_FALSE(win.SplitLabel(1)->isVisible());
  EXPECT_FALSE(win.SplitLabel(2)->isVisible());
  EXPECT_FALSE(win.SplitLabel(3)->isVisible());

  // Split 2 (2 files): even offsets black, odd offsets olive (#808000).
  win.SetSplitIndex(1);
  EXPECT_EQ(win.Split(), 2);
  EXPECT_EQ(win.ByteColor(0x1000), QColor(Qt::black));
  EXPECT_EQ(win.ByteColor(0x1001), QColor(0x80, 0x80, 0x00));
  EXPECT_EQ(win.ByteColor(0x1002), QColor(Qt::black));
  EXPECT_EQ(win.ByteColor(0x1003), QColor(0x80, 0x80, 0x00));
  EXPECT_TRUE(win.SplitLabel(0)->isVisible());
  EXPECT_TRUE(win.SplitLabel(1)->isVisible());
  EXPECT_FALSE(win.SplitLabel(2)->isVisible());
  EXPECT_FALSE(win.SplitLabel(3)->isVisible());

  // Split 4 (4 files): black, olive, green, blue.
  win.SetSplitIndex(2);
  EXPECT_EQ(win.Split(), 4);
  EXPECT_EQ(win.ByteColor(0x1000), QColor(Qt::black));
  EXPECT_EQ(win.ByteColor(0x1001), QColor(0x80, 0x80, 0x00));
  EXPECT_EQ(win.ByteColor(0x1002), QColor(0x00, 0x80, 0x00));
  EXPECT_EQ(win.ByteColor(0x1003), QColor(0x00, 0x00, 0xFF));
  EXPECT_TRUE(win.SplitLabel(0)->isVisible());
  EXPECT_TRUE(win.SplitLabel(1)->isVisible());
  EXPECT_TRUE(win.SplitLabel(2)->isVisible());
  EXPECT_TRUE(win.SplitLabel(3)->isVisible());
}

TEST(EasyBinTest, SaveAndLoadBinarySplits1And2And4) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  MainWindow win;
  win.SetNotify([](const QString&) {});
  const uint8_t sample[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
  for (int i = 0; i < 8; ++i) {
    win.GetMemory().RawData()[0x2000 + i] = sample[i];
  }
  win.OutputFirstAddressField()->setText(QStringLiteral("002000"));
  win.OutputLengthField()->setText(QStringLiteral("000008"));

  // 1-file save.
  win.SetSplitIndex(0);
  const QString bin1 = temp_dir.filePath(QStringLiteral("single.bin"));
  ASSERT_TRUE(win.SaveBinaryTo(bin1));
  EXPECT_EQ(ReadAllBytes(bin1), QByteArray(reinterpret_cast<const char*>(sample), 8));

  // 2-file split save.
  win.SetSplitIndex(1);
  const QString bin2 = temp_dir.filePath(QStringLiteral("pair.bin"));
  ASSERT_TRUE(win.SaveBinaryTo(bin2));
  EXPECT_EQ(ReadAllBytes(temp_dir.filePath(QStringLiteral("pair_0.bin"))),
            QByteArray("\x11\x33\x55\x77", 4));
  EXPECT_EQ(ReadAllBytes(temp_dir.filePath(QStringLiteral("pair_1.bin"))),
            QByteArray("\x22\x44\x66\x88", 4));

  // 4-file split save.
  win.SetSplitIndex(2);
  const QString bin4 = temp_dir.filePath(QStringLiteral("quad.bin"));
  ASSERT_TRUE(win.SaveBinaryTo(bin4));
  EXPECT_EQ(ReadAllBytes(temp_dir.filePath(QStringLiteral("quad_0.bin"))),
            QByteArray("\x11\x55", 2));
  EXPECT_EQ(ReadAllBytes(temp_dir.filePath(QStringLiteral("quad_1.bin"))),
            QByteArray("\x22\x66", 2));
  EXPECT_EQ(ReadAllBytes(temp_dir.filePath(QStringLiteral("quad_2.bin"))),
            QByteArray("\x33\x77", 2));
  EXPECT_EQ(ReadAllBytes(temp_dir.filePath(QStringLiteral("quad_3.bin"))),
            QByteArray("\x44\x88", 2));

  // Reloading pair_0.bin and pair_1.bin with 2-way split reassembles the 8 bytes.
  win.NewEnvironment(/*confirm=*/false);
  win.SetSplitIndex(1);
  win.OutputFirstAddressField()->setText(QStringLiteral("003000"));
  EXPECT_EQ(win.OpenBinaryFile(temp_dir.filePath(QStringLiteral("pair_0.bin"))), 8);
  win.OutputFirstAddressField()->setText(QStringLiteral("003001"));
  EXPECT_EQ(win.OpenBinaryFile(temp_dir.filePath(QStringLiteral("pair_1.bin"))), 8);
  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(win.GetMemory().RawData()[0x3000 + i], sample[i]);
  }
}

TEST(EasyBinTest, SaveSRecordRoundTrip) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const QString s68_path = temp_dir.filePath(QStringLiteral("roundtrip.S68"));

  MainWindow win;
  win.SetNotify([](const QString&) {});
  for (uint32_t i = 0; i < 40; ++i) {
    win.GetMemory().RawData()[0x010200 + i] = static_cast<uint8_t>(0xA0 + i);
  }
  win.FromField()->setText(QStringLiteral("010200"));
  win.ToField()->setText(QStringLiteral("010227"));
  win.StartAddressField()->setText(QStringLiteral("00010200"));
  ASSERT_TRUE(win.SaveSRecordTo(s68_path));

  win.NewEnvironment(/*confirm=*/false);
  ASSERT_TRUE(win.OpenSRecordFile(s68_path));
  EXPECT_EQ(win.StartAddressField()->text(), QStringLiteral("00010200"));
  EXPECT_EQ(win.S68EndAddressLabel()->text(), QStringLiteral("010227"));
  EXPECT_EQ(win.S68LengthLabel()->text(), QStringLiteral("000028"));
  for (uint32_t i = 0; i < 40; ++i) {
    EXPECT_EQ(win.GetMemory().RawData()[0x010200 + i], static_cast<uint8_t>(0xA0 + i));
  }
}

TEST(EasyBinTest, HexAndAsciiEditingCopyFillAndNavigation) {
  MainWindow win;
  win.SetNotify([](const QString&) {});
  win.show();
  win.ShowAddress(0x000400);
  EXPECT_EQ(win.TopAddress(), 0x000400u);

  // Edit high and low nybbles in hex area at 0x000400.
  win.PlaceCursor(0, MainWindow::kHexLeft);
  QTest::keyClick(&win, Qt::Key_A);
  QTest::keyClick(&win, Qt::Key_5);
  EXPECT_EQ(win.GetMemory().RawData()[0x000400], 0xA5);

  // Edit byte in ASCII area at 0x000401.
  win.PlaceCursor(0, MainWindow::kAscLeft + 1);
  QTest::keyClicks(&win, QStringLiteral("Q"));
  EXPECT_EQ(win.GetMemory().RawData()[0x000401], static_cast<uint8_t>('Q'));
  EXPECT_TRUE(win.RowText(0).startsWith(QStringLiteral("00000400: A5 51 ")));

  // Fill range $000500..$00050F with $3C.
  win.FromField()->setText(QStringLiteral("000500"));
  win.ToField()->setText(QStringLiteral("00050F"));
  win.FillByteField()->setText(QStringLiteral("3C"));
  win.FillRange();
  for (uint32_t a = 0x500; a <= 0x50F; ++a) {
    EXPECT_EQ(win.GetMemory().RawData()[a], 0x3C);
  }

  // Overlapping copy: set $000600..$000603 = 01 02 03 04, copy 4 bytes to $000602.
  for (uint32_t i = 0; i < 4; ++i) {
    win.GetMemory().RawData()[0x600 + i] = static_cast<uint8_t>(i + 1);
  }
  win.FromField()->setText(QStringLiteral("000600"));
  win.ToField()->setText(QStringLiteral("000602"));
  win.BytesField()->setText(QStringLiteral("000004"));
  win.CopyRange();
  EXPECT_EQ(win.GetMemory().RawData()[0x600], 1);
  EXPECT_EQ(win.GetMemory().RawData()[0x601], 2);
  EXPECT_EQ(win.GetMemory().RawData()[0x602], 1);
  EXPECT_EQ(win.GetMemory().RawData()[0x603], 2);
  EXPECT_EQ(win.GetMemory().RawData()[0x604], 3);
  EXPECT_EQ(win.GetMemory().RawData()[0x605], 4);
}

TEST(EasyBinTest, DropFileSRecordAndBinaryFallback) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const QString bin_path = temp_dir.filePath(QStringLiteral("raw.bin"));
  {
    QFile f(bin_path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("\xDE\xAD\xBE\xEF", 4);
  }

  MainWindow win;
  win.SetNotify([](const QString&) {});
  win.OutputFirstAddressField()->setText(QStringLiteral("000800"));
  ASSERT_TRUE(win.DropFile(bin_path));
  EXPECT_TRUE(win.Notifications().contains(QStringLiteral("Opening as binary file.")));
  EXPECT_EQ(win.OutputLengthField()->text(), QStringLiteral("000004"));
  EXPECT_EQ(win.GetMemory().RawData()[0x800], 0xDE);
  EXPECT_EQ(win.GetMemory().RawData()[0x801], 0xAD);
  EXPECT_EQ(win.GetMemory().RawData()[0x802], 0xBE);
  EXPECT_EQ(win.GetMemory().RawData()[0x803], 0xEF);
}

TEST(EasyBinTest, Sim68KViewMenuLaunchesEasyBin) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  easym68k::gui::MainWindow sim_win(temp_dir.filePath(QStringLiteral("sim68k.ini")));
  QString launched_path;
  sim_win.SetEasyBinLauncher([&](const QString& path) {
    launched_path = path;
    return true;
  });
  ASSERT_NE(sim_win.ViewEasyBinAction(), nullptr);
  EXPECT_TRUE(sim_win.ViewEasyBinAction()->isEnabled());
  sim_win.ViewEasyBinAction()->trigger();
  EXPECT_FALSE(launched_path.isEmpty());
  EXPECT_TRUE(launched_path.endsWith(QStringLiteral("easybin")) ||
              launched_path.endsWith(QStringLiteral("easybin.exe")));
}

}  // namespace
}  // namespace easym68k::easybin
