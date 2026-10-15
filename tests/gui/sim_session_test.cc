// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// SimSession (the Sim68K-Qt session model) with programs assembled by the
// port's assembler: loading, *[sim68k] commands, Run/Trace/Step Over/Pause/
// Rewind/Close on the worker thread, messages, STOP and interrupts.

#include "sim_session.h"

#include <gtest/gtest.h>

#include <chrono>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <string>
#include <thread>

#include "easym68k/asm/assembler.h"
#include "listing.h"
#include "sim_io_screen.h"

namespace easym68k::gui {
namespace {

using RunState = SimSession::RunState;

template <typename Pred>
bool Until(Pred done, int timeout_ms = 5000) {
  QElapsedTimer t;
  t.start();
  while (!done()) {
    if (t.elapsed() > timeout_ms)
      return false;
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
  }
  return true;
}

class SimSessionTest : public ::testing::Test {
 protected:
  // Assembles `source` to <name>.S68 / .L68 in the temporary directory.
  QString Assemble(const QString& name, const std::string& source, bool listing = true) {
    asm_::Assembler assembler;
    assembler.ConfigureListing(name.toStdString() + ".X68", "");
    auto r = assembler.Assemble(source);
    EXPECT_TRUE(r.success) << (r.errors.empty() ? "" : r.errors[0].message);
    const QString base = dir_.filePath(name);
    QFile s68(base + QStringLiteral(".S68"));
    s68.open(QIODevice::WriteOnly);
    s68.write(r.srecord.c_str());
    if (listing) {
      QFile l68(base + QStringLiteral(".L68"));
      l68.open(QIODevice::WriteOnly);
      l68.write(assembler.GetListing().c_str());
    }
    return base + QStringLiteral(".S68");
  }

  bool WaitState(SimSession& s, RunState state) {
    return Until([&] { return s.State() == state; });
  }

  QTemporaryDir dir_;
};

const char kHello[] =
    "        ORG     $1000\n"
    "START   LEA     MSG,A1\n"
    "        MOVE.B  #14,D0\n"
    "        TRAP    #15\n"
    "        SIMHALT\n"
    "MSG     DC.B    'Hi',0\n"
    "        END     START\n";

TEST_F(SimSessionTest, OpenSetsPcFromTheListingAndRuns) {
  SimSession s;
  QSignalSpy loaded(&s, &SimSession::ProgramLoaded);
  EXPECT_EQ(s.State(), RunState::kNoProgram);
  ASSERT_TRUE(s.Open(Assemble("hello", kHello)));
  EXPECT_EQ(loaded.count(), 1);
  EXPECT_EQ(s.State(), RunState::kReady);
  EXPECT_TRUE(s.HasListing());
  EXPECT_EQ(s.Simulator().State().pc, 0x1000u);
  EXPECT_EQ(s.Simulator().GetMemory().RawData()[0x8000], 0xFF);  // erased to $FF
  s.Run();
  EXPECT_EQ(s.State(), RunState::kRunning);
  ASSERT_TRUE(WaitState(s, RunState::kHalted));
  EXPECT_EQ(s.Screen().CharAt(0, 0), 'H');
  EXPECT_EQ(s.Screen().CharAt(0, 1), 'i');
  s.Run();  // halted: Run is not available
  EXPECT_EQ(s.State(), RunState::kHalted);
  s.Rewind();
  EXPECT_EQ(s.State(), RunState::kReady);
  EXPECT_EQ(s.Simulator().State().pc, 0x1000u);
  EXPECT_EQ(s.Screen().CharAt(0, 0), ' ');  // the output window was cleared
}

TEST_F(SimSessionTest, ListingCommandsAndBreakpointMessage) {
  const std::string source =
      "        ORG     $1000\n"
      "START   NOP\n"
      "*[sim68k]break\n"
      "        NOP\n"
      "        NOP\n"
      "*[sim68k]bitfield\n"
      "        SIMHALT\n"
      "        END     START\n";
  SimSession s;
  QSignalSpy messages(&s, &SimSession::Message);
  ASSERT_TRUE(s.Open(Assemble("brk", source)));
  EXPECT_TRUE(s.HasBreakpoint(0x1002));
  EXPECT_TRUE(s.Simulator().HasBreakpoint(0x1002));
  EXPECT_TRUE(s.BitFieldEnabled());
  s.Run();
  ASSERT_TRUE(Until([&] { return !s.IsRunning(); }));
  EXPECT_EQ(s.State(), RunState::kReady);
  EXPECT_EQ(s.Simulator().State().pc, 0x1002u);
  ASSERT_GE(messages.count(), 1);
  EXPECT_EQ(messages.last().at(0).toString(), QStringLiteral("break point at 1002"));
  s.TraceInto();
  ASSERT_TRUE(Until([&] { return !s.IsRunning(); }));
  EXPECT_EQ(s.Simulator().State().pc, 0x1004u);
  // Reload keeps breakpoints; Open clears them (then re-adds *[sim68k]break).
  s.SetBreakpoint(0x1006, true);
  ASSERT_TRUE(s.Reload());
  EXPECT_TRUE(s.HasBreakpoint(0x1006));
  EXPECT_TRUE(s.Simulator().HasBreakpoint(0x1006));
  ASSERT_TRUE(s.Open(s.ProgramPath()));
  EXPECT_FALSE(s.HasBreakpoint(0x1006));
  EXPECT_FALSE(s.Simulator().HasBreakpoint(0x1006));
  EXPECT_TRUE(s.HasBreakpoint(0x1002));
}

TEST_F(SimSessionTest, BreakpointSetWhileRunningStopsTheProgram) {
  const std::string source =
      "        ORG     $1000\n"
      "START   ADDQ.L  #1,D0\n"
      "        BRA     START\n"
      "        END     START\n";
  SimSession s;
  ASSERT_TRUE(s.Open(Assemble("bprun", source)));
  s.Run();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_TRUE(s.IsRunning());
  s.ToggleBreakpoint(0x1002);  // reaches the worker before its next instruction
  ASSERT_TRUE(Until([&] { return !s.IsRunning(); }));
  EXPECT_EQ(s.Simulator().State().pc, 0x1002u);
  EXPECT_TRUE(s.Simulator().HasBreakpoint(0x1002));
  s.ToggleBreakpoint(0x1002);
  EXPECT_FALSE(s.HasBreakpoint(0x1002));
  EXPECT_FALSE(s.Simulator().HasBreakpoint(0x1002));
  s.SetBreakpoint(0x1000, true);
  s.ClearBreakpoints();
  EXPECT_TRUE(s.Breakpoints().empty());
  EXPECT_FALSE(s.Simulator().HasBreakpoint(0x1000));
}

TEST_F(SimSessionTest, MemorySnapshotWhileRunning) {
  const std::string source =
      "        ORG     $1000\n"
      "START   ADDQ.L  #1,D0\n"
      "        BRA     START\n"
      "        END     START\n";
  SimSession s;
  ASSERT_TRUE(s.Open(Assemble("snap", source)));
  QSignalSpy snaps(&s, &SimSession::MemorySnapshot);
  s.Run();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  s.RequestMemorySnapshot(0x1000, 4);  // copied by the worker
  ASSERT_TRUE(Until([&] { return snaps.count() == 1; }));
  EXPECT_EQ(snaps.last().at(0).toUInt(), 0x1000u);
  EXPECT_EQ(snaps.last().at(1).toByteArray(), QByteArray("\x52\x80\x60\xFC", 4));
  s.Pause();
  ASSERT_TRUE(Until([&] { return !s.IsRunning(); }));
  s.RequestMemorySnapshot(0x1002, 2);  // stopped: copied at once
  ASSERT_TRUE(Until([&] { return snaps.count() == 2; }));
  EXPECT_EQ(snaps.last().at(1).toByteArray(), QByteArray("\x60\xFC", 2));
}

TEST_F(SimSessionTest, StepOverRunsASubroutine) {
  const std::string source =
      "        ORG     $1000\n"
      "START   BSR     SUB\n"
      "        SIMHALT\n"
      "SUB     MOVEQ   #5,D0\n"
      "        RTS\n"
      "        END     START\n";
  SimSession s;
  ASSERT_TRUE(s.Open(Assemble("sub", source)));
  s.StepOver();
  ASSERT_TRUE(Until([&] { return !s.IsRunning(); }));
  EXPECT_EQ(s.Simulator().State().pc, 0x1004u);  // after BSR.W
  EXPECT_EQ(s.Simulator().State().d[0], 5u);
  EXPECT_EQ(s.State(), RunState::kReady);
}

TEST_F(SimSessionTest, PauseReleasesAProgramWaitingForInput) {
  const std::string source =
      "        ORG     $1000\n"
      "START   MOVE.B  #4,D0\n"
      "        TRAP    #15\n"
      "        SIMHALT\n"
      "        END     START\n";
  SimSession s;
  ASSERT_TRUE(s.Open(Assemble("input", source)));
  s.Run();
  ASSERT_TRUE(Until([&] { return s.Screen().InputActive(); }));
  s.Pause();
  ASSERT_TRUE(Until([&] { return !s.IsRunning(); }));
  EXPECT_EQ(s.State(), RunState::kReady);
  EXPECT_EQ(s.Simulator().State().pc, 0x1006u);  // after the TRAP
  s.Run();
  ASSERT_TRUE(WaitState(s, RunState::kHalted));
}

TEST_F(SimSessionTest, IllegalInstructionWithExceptionsOff) {
  const std::string source =
      "        ORG     $1000\n"
      "START   NOP\n"
      "        DC.W    $4AFC\n"
      "        END     START\n";
  SimSession s;
  EXPECT_FALSE(s.ExceptionsEnabled());  // Sim68K default
  QSignalSpy messages(&s, &SimSession::Message);
  ASSERT_TRUE(s.Open(Assemble("illegal", source)));
  s.Run();
  ASSERT_TRUE(WaitState(s, RunState::kHalted));
  ASSERT_GE(messages.count(), 1);
  EXPECT_EQ(messages.last().at(0).toString(),
            QStringLiteral("Illegal instruction found at location 1002. Execution halted"));
}

TEST_F(SimSessionTest, StopWaitsForAnInterrupt) {
  const std::string source =
      "        ORG     $1000\n"
      "START   MOVE.L  #HANDLER,$64\n"
      "        STOP    #$2000\n"
      "        SIMHALT\n"
      "HANDLER MOVEQ   #7,D7\n"
      "        RTE\n"
      "        END     START\n";
  SimSession s;
  s.SetExceptionsEnabled(true);
  QSignalSpy messages(&s, &SimSession::Message);
  ASSERT_TRUE(s.Open(Assemble("stop", source)));
  s.Run();
  ASSERT_TRUE(WaitState(s, RunState::kStopped));
  EXPECT_EQ(messages.last().at(0).toString(), QStringLiteral("STOP instruction. Execution halted"));
  s.Interrupt(1);  // the hardware window's IRQ 1: the program continues
  ASSERT_TRUE(WaitState(s, RunState::kHalted));
  EXPECT_EQ(s.Simulator().State().d[7], 7u);
}

TEST_F(SimSessionTest, RewindAndCloseWhileRunning) {
  const std::string source =
      "        ORG     $1000\n"
      "START   ADDQ.L  #1,D0\n"
      "        BRA     START\n"
      "        END     START\n";
  SimSession s;
  ASSERT_TRUE(s.Open(Assemble("loop", source)));
  s.Run();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  s.Rewind();  // stops first, then rewinds
  ASSERT_TRUE(Until([&] { return !s.IsRunning(); }));
  EXPECT_EQ(s.State(), RunState::kReady);
  EXPECT_EQ(s.Simulator().State().pc, 0x1000u);
  EXPECT_EQ(s.Simulator().State().d[0], 0u);
  s.Run();
  QSignalSpy closed(&s, &SimSession::ProgramClosed);
  s.Close();
  ASSERT_TRUE(Until([&] { return closed.count() == 1; }));
  EXPECT_TRUE(s.Listing().isEmpty());
  EXPECT_FALSE(s.ProgramPath().isEmpty());  // kept for Reload
}

TEST_F(SimSessionTest, MissingListingAndMissingFile) {
  SimSession s;
  QSignalSpy messages(&s, &SimSession::Message);
  ASSERT_TRUE(s.Open(Assemble("nolst", kHello, /*listing=*/false)));
  EXPECT_FALSE(s.HasListing());
  EXPECT_EQ(messages.last().at(0).toString(), QStringLiteral("Unable to open .L68 file."));
  EXPECT_TRUE(s.Listing().first().startsWith(QStringLiteral("A matching .L68 file")));
  EXPECT_EQ(s.Simulator().State().pc, 0x1000u);  // from the S-record
  EXPECT_FALSE(s.Open(dir_.filePath(QStringLiteral("none.S68"))));
  EXPECT_TRUE(
      messages.last().at(0).toString().startsWith(QStringLiteral("error: cannot open file")));
  EXPECT_EQ(s.State(), RunState::kNoProgram);
}

TEST(ListingTest, InstructionLinesAndCommands) {
  const QStringList lines = {
      QStringLiteral("00001000 Starting Address"),
      QStringLiteral("Assembler used: EASy68K Editor/Assembler v5.16.01"),
      QStringLiteral("Created On: "),
      QStringLiteral("00001000  4E71                       4  START   NOP"),
      QStringLiteral("00001002                             5  *[SIM68K]BREAK"),
      QStringLiteral("00001002  =0000000E                  6  VAL     EQU     14"),
      QStringLiteral("00001002  4E71                       7          NOP"),
      QStringLiteral("00001004= 01                         8          DC.B    1"),
      QStringLiteral("00001006                             9  *[sim68k]simhalt_off"),
      QStringLiteral("00001006                            10  *[sim68k]break"),
  };
  EXPECT_TRUE(IsInstructionLine(lines[3]));
  EXPECT_FALSE(IsInstructionLine(lines[4]));
  EXPECT_FALSE(IsInstructionLine(lines[5]));  // EQU
  EXPECT_FALSE(IsInstructionLine(lines[7]));  // DC
  const ListingCommands c = ParseListing(lines);
  ASSERT_TRUE(c.start_address.has_value());
  EXPECT_EQ(*c.start_address, 0x1000u);
  EXPECT_TRUE(c.breakpoints.empty());  // the next line is EQU; a break on the last line is ignored
  EXPECT_TRUE(c.simhalt_off);
  EXPECT_FALSE(c.bitfield);
  EXPECT_EQ(LineOfAddress(lines, 0x1002), 6);
  EXPECT_EQ(LineOfAddress(lines, 0x2000), -1);
}

}  // namespace
}  // namespace easym68k::gui
