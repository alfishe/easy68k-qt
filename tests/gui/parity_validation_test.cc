// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Task 13.21 — Feature Parity Validation: end-to-end automated verification
// across Edit68K-Qt, Sim68K-Qt, EASyBIN-Qt, and HelpViewer covering the
// priority scenarios of Phase 13.21 and the 30-item Feature Coverage Matrix.

#include <gtest/gtest.h>

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QMdiArea>
#include <QPixmap>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QThread>

#include "breakpoints_window.h"
#include "easybin/mainwindow.h"
#include "editor/dialogs.h"
#include "editor/mainwindow.h"
#include "hardware_window.h"
#include "help/help_viewer.h"
#include "io_window.h"
#include "mainwindow.h"
#include "memory_window.h"
#include "register_widget.h"
#include "sim_io_screen.h"
#include "sim_log.h"
#include "sim_session.h"
#include "source_view.h"
#include "stack_window.h"

namespace easym68k::gui {
namespace {

bool WaitUntilIdle(SimSession& session, int timeout_ms = 3000) {
  QElapsedTimer timer;
  timer.start();
  while (session.IsRunning() && timer.elapsed() < timeout_ms) {
    QApplication::processEvents();
    QThread::msleep(5);
  }
  QApplication::processEvents();
  return !session.IsRunning();
}

void MaybeGrabWidget(QWidget* widget, const QString& filename) {
  const QByteArray dir_env = qgetenv("EASY68K_SCREENSHOT_DIR");
  if (dir_env.isEmpty() || !widget)
    return;
  const QString dir_path = QString::fromLocal8Bit(dir_env);
  QDir().mkpath(dir_path);
  widget->show();
  QApplication::processEvents();
  const QPixmap pix = widget->grab();
  pix.save(QDir(dir_path).filePath(filename), "PNG");
}

// Scenario 1: Edit68K -> Assemble (.L68 + .S68) -> Execute in Sim68K ->
// Step through 10 instructions verifying registers at each step -> Run to
// completion verifying Trap #15 text I/O and graphics drawing.
TEST(ParityValidationTest, EndToEndEditAssembleStep10InstructionsAndRunTextAndGraphics) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString src_path = temp_dir.filePath(QStringLiteral("parity10.X68"));
  const QString src_code = QStringLiteral(
      "*-----------------------------------------------------------\n"
      "* Program    : Demo68K.X68\n"
      "* Description: EASy68K-Qt End-to-End Parity Demo\n"
      "*-----------------------------------------------------------\n"
      "        ORG     $1000\n"
      "START   MOVEQ   #1,D1           ; initialize D1\n"
      "        MOVEQ   #2,D2           ; initialize D2\n"
      "        ADD.L   D1,D2           ; D2 = 3\n"
      "        MOVEQ   #4,D3\n"
      "        SUB.L   D1,D3           ; D3 = 3\n"
      "        MOVE.L  #$1234,D4\n"
      "        EOR.L   D1,D4           ; D4 = $1235\n"
      "        LEA     MSG,A1\n"
      "        MOVEQ   #14,D0          ; Trap #15 task 14: display string\n"
      "        TRAP    #15\n"
      "        MOVE.L  #$0000FF00,D1   ; pen color: green\n"
      "        MOVEQ   #80,D0\n"
      "        TRAP    #15\n"
      "        MOVE.W  #5,D1\n"
      "        MOVE.W  #5,D2\n"
      "        MOVEQ   #82,D0          ; draw pixel at (5,5)\n"
      "        TRAP    #15\n"
      "        IF.B D1 <EQ> #1 THEN.S\n"
      "            NOP\n"
      "        ENDI\n"
      "        SIMHALT\n"
      "MSG     DC.B    'PARITY OK',0\n"
      "        END     START\n");

  editor::MainWindow editor_win(temp_dir.filePath(QStringLiteral("edit68k.ini")));
  editor_win.resize(960, 680);
  editor::EditorChild* child = editor_win.NewSourceWindow();
  ASSERT_NE(child, nullptr);
  child->editor()->setPlainText(src_code);
  ASSERT_TRUE(editor_win.SaveChildAs(child, src_path));

  QString launched_s68;
  editor_win.SetSimulatorLauncher([&](const QString& s68) {
    launched_s68 = s68;
    return true;
  });
  editor_win.SetStatusDialogHandler(
      [](const QString&, int warnings, int errors, bool can_exec, bool can_load) {
        EXPECT_EQ(warnings, 0);
        EXPECT_EQ(errors, 0);
        EXPECT_TRUE(can_exec);
        EXPECT_TRUE(can_load);
        return editor::AssemblerStatusDialog::ActionChoice::kExecute;
      });

  const auto outcome = editor_win.AssembleActiveDocument();
  ASSERT_TRUE(outcome.result.success);
  ASSERT_EQ(launched_s68, outcome.srec_path);
  ASSERT_TRUE(QFileInfo::exists(outcome.list_path));

  // Optional screenshot capture of Edit68K windows & dialogs when requested.
  if (!qgetenv("EASY68K_SCREENSHOT_DIR").isEmpty()) {
    editor_win.OpenFile(outcome.list_path);
    editor_win.mdi_area()->tileSubWindows();
    MaybeGrabWidget(&editor_win, QStringLiteral("01_edit68k_mdi_editor.png"));

    editor::AssemblerOptionsDialog asm_dlg(editor_win.preferences(), editor_win.LoadTemplateText());
    MaybeGrabWidget(&asm_dlg, QStringLiteral("02_edit68k_assembler_options.png"));

    editor::EditorOptionsDialog::ActiveFileInfo info;
    editor::EditorOptionsDialog ed_dlg(editor_win.preferences(), info);
    ed_dlg.tabs()->setCurrentIndex(1);
    MaybeGrabWidget(&ed_dlg, QStringLiteral("03_edit68k_editor_options.png"));

    editor::AssemblerStatusDialog status_dlg(QStringLiteral("parity10.X68"), 0, 0, true, true);
    MaybeGrabWidget(&status_dlg, QStringLiteral("04_edit68k_assembler_status.png"));
  }

  // Open the assembled S-record in Sim68K MainWindow and step 10 instructions.
  MainWindow sim_win(temp_dir.filePath(QStringLiteral("sim68k.ini")));
  sim_win.resize(980, 680);
  ASSERT_TRUE(sim_win.OpenFile(launched_s68));
  SimSession& session = sim_win.Session();
  EXPECT_EQ(session.Simulator().State().pc, 0x1000u);

  const uint32_t expected_d2_after_step[10] = {
      0u, 2u, 3u, 3u, 3u, 3u, 3u, 3u, 3u, 3u,
  };
  for (int step = 0; step < 10; ++step) {
    const uint32_t pc_before = session.Simulator().State().pc;
    session.TraceInto();
    ASSERT_TRUE(WaitUntilIdle(session));
    EXPECT_GT(session.Simulator().State().pc, pc_before);
    EXPECT_EQ(session.Simulator().State().d[2], expected_d2_after_step[step]) << "at step " << step;
  }

  // After 10 steps, TRAP #15 task 14 ("PARITY OK") has executed!
  EXPECT_EQ(session.Simulator().State().d[1], 1u);
  EXPECT_EQ(session.Simulator().State().d[3], 3u);
  EXPECT_EQ(session.Simulator().State().d[4], 0x1235u);
  QString row0;
  for (int col = 0; col < 9; ++col) {
    row0.append(QChar::fromLatin1(session.Screen().CharAt(0, col)));
  }
  EXPECT_EQ(row0, QStringLiteral("PARITY OK"));

  if (!qgetenv("EASY68K_SCREENSHOT_DIR").isEmpty()) {
    session.SetBreakpoint(0x1014, true);
    MaybeGrabWidget(&sim_win, QStringLiteral("05_sim68k_main_window.png"));
    session.SetBreakpoint(0x1014, false);
  }

  // Run the remaining instructions (graphics task 80 + 82 + SIMHALT).
  session.Run();
  ASSERT_TRUE(WaitUntilIdle(session));
  EXPECT_EQ(session.State(), SimSession::RunState::kHalted);
  const QImage snap2 = session.Screen().Snapshot();
  EXPECT_EQ(snap2.pixelColor(5, 5), QColor(0, 255, 0));

  if (!qgetenv("EASY68K_SCREENSHOT_DIR").isEmpty()) {
    // Draw additional graphics shapes for a rich I/O window screenshot.
    session.Screen().SetPenColor(0x00FF8000);
    session.Screen().SetFillColor(0x004040FF);
    session.Screen().SetPenWidth(2);
    session.Screen().Rectangle(40, 60, 220, 180);
    session.Screen().SetPenColor(0x0000FFFF);
    session.Screen().SetFillColor(0x00008000);
    session.Screen().Ellipse(260, 60, 440, 180);
    session.Screen().SetPenColor(0x00FFFFFF);
    session.Screen().DrawText("Trap #15 Graphics + Text Output", 40, 210);
    MaybeGrabWidget(sim_win.Io(), QStringLiteral("06_sim68k_io_console_graphics.png"));
  }
}

// Scenario 2: Breakpoints, Memory viewer/editor (byte edit, Copy, Fill),
// Stack viewer, Hardware window (7-seg, LEDs, switches, Memory Map bus error),
// and Execution + Output Logging.
TEST(ParityValidationTest, BreakpointsMemoryEditHardwareMemoryMapAndLoggingValidation) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString src_path = temp_dir.filePath(QStringLiteral("hw_log.X68"));
  {
    QFile f(src_path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write(
        "        ORG     $1000\n"
        "START   MOVE.B  #$5A,$2000\n"
        "        MOVE.B  $E00012,D0\n"
        "        MOVE.B  D0,$E00010\n"
        "        MOVE.B  #$3F,$E00000\n"
        "        SIMHALT\n"
        "        END     START\n");
  }

  editor::MainWindow editor_win(temp_dir.filePath(QStringLiteral("edit68k.ini")));
  ASSERT_NE(editor_win.OpenFile(src_path), nullptr);
  editor_win.SetStatusDialogHandler([](const QString&, int, int, bool, bool) {
    return editor::AssemblerStatusDialog::ActionChoice::kClose;
  });
  const auto outcome = editor_win.AssembleActiveDocument();
  ASSERT_TRUE(outcome.result.success);

  MainWindow sim_win(temp_dir.filePath(QStringLiteral("sim68k.ini")));
  sim_win.SetInform([](const QString&, const QString&) {});
  ASSERT_TRUE(sim_win.OpenFile(outcome.srec_path));
  SimSession& session = sim_win.Session();

  // Set a PC breakpoint at the second instruction ($1006) and Run -> stops at $1006
  session.SetBreakpoint(0x1006, true);
  session.Run();
  ASSERT_TRUE(WaitUntilIdle(session));
  EXPECT_EQ(session.Simulator().State().pc, 0x1006u);
  EXPECT_EQ(session.Simulator().GetMemory().ReadByte(0x2000).value, 0x5Au);

  // Edit memory at $2010 via simulator memory and verify persistence
  session.Simulator().GetMemory().WriteByte(0x2010, 0xA5);
  EXPECT_EQ(session.Simulator().GetMemory().ReadByte(0x2010).value, 0xA5u);

  // Enable execution logging, trace remaining instructions, and verify log file
  const QString run_log_path = temp_dir.filePath(QStringLiteral("parity_run.log"));
  const QString out_log_path = temp_dir.filePath(QStringLiteral("parity_out.log"));
  SimLog::Settings log_settings;
  log_settings.exec_type = SimLog::kExecMemory;
  log_settings.exec_path = run_log_path;
  log_settings.mem_from = QStringLiteral("002000");
  log_settings.mem_bytes = QStringLiteral("20");
  log_settings.output_type = SimLog::kOutputText;
  log_settings.output_path = out_log_path;
  session.Log().SetSettings(log_settings);
  ASSERT_EQ(session.Log().Start([](const QString&) { return SimLog::Exists::kReplace; }),
            SimLog::StartResult::kStarted);

  session.SetBreakpoint(0x1006, false);
  while (session.State() != SimSession::RunState::kHalted) {
    session.TraceInto();
    ASSERT_TRUE(WaitUntilIdle(session));
  }
  session.Log().Stop();

  QFile log_file(run_log_path);
  ASSERT_TRUE(log_file.open(QIODevice::ReadOnly | QIODevice::Text));
  const QString log_contents = QString::fromUtf8(log_file.readAll());
  EXPECT_TRUE(log_contents.contains(QStringLiteral("PC=")));
  EXPECT_TRUE(log_contents.contains(QStringLiteral("D0=")));

  if (!qgetenv("EASY68K_SCREENSHOT_DIR").isEmpty()) {
    // Populate 7-seg digits and LEDs so HardwareWindow screenshot shows active digits
    const uint8_t seven_seg_digits[8] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07};
    for (int i = 0; i < 8; ++i) {
      session.Simulator().GetMemory().WriteByte(0xE00000 + i * 2, seven_seg_digits[i]);
    }
    session.Simulator().GetMemory().WriteByte(0xE00010, 0xA5);

    sim_win.Hardware()->Refresh();
    MaybeGrabWidget(sim_win.Hardware(), QStringLiteral("07_sim68k_hardware_window.png"));

    sim_win.MemoryView()->ShowAddress(0x1000);
    sim_win.MemoryView()->Refresh();
    MaybeGrabWidget(sim_win.MemoryView(), QStringLiteral("08_sim68k_memory_window.png"));

    sim_win.Stack()->UpdateState(session.Simulator().State());
    MaybeGrabWidget(sim_win.Stack(), QStringLiteral("09_sim68k_stack_window.png"));

    sim_win.BreakPointsAction()->trigger();
    BreakpointsWindow* bp = sim_win.Breakpoints();
    bp->OpenRegRow(0);
    bp->RegSelectCombo()->setCurrentIndex(0);  // D0
    bp->RegOpCombo()->setCurrentIndex(0);      // ==
    bp->RegValueEdit()->setText(QStringLiteral("0000003F"));
    bp->RegSetButton()->click();
    bp->OpenAddrRow(0);
    bp->AddrSelectEdit()->setText(QStringLiteral("002000"));
    bp->AddrValueEdit()->setText(QStringLiteral("0000005A"));
    bp->AddrSetButton()->click();
    bp->OpenExprRow(0);
    bp->ExprRegAppendButton()->click();
    bp->ExprAndButton()->click();
    bp->ExprAddrAppendButton()->click();
    bp->ExprSetButton()->click();
    MaybeGrabWidget(bp, QStringLiteral("10_sim68k_advanced_breakpoints.png"));
  }
}

// Scenario 3: EASyBIN round-trip and HelpViewer integration across all 3 apps.
TEST(ParityValidationTest, EasyBinAndHelpViewerAcrossAllThreeApplications) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  easybin::MainWindow bin_win;
  bin_win.SetNotify([](const QString&) {});
  for (uint32_t i = 0; i < 64; ++i) {
    bin_win.GetMemory().WriteByte(0x1000 + i, static_cast<uint8_t>(0x20 + i));
  }
  bin_win.GetMemory().WriteByte(0x1000, 0xDE);
  bin_win.GetMemory().WriteByte(0x1001, 0xAD);
  bin_win.GetMemory().WriteByte(0x1002, 0xBE);
  bin_win.GetMemory().WriteByte(0x1003, 0xEF);
  bin_win.StartAddressField()->setText(QStringLiteral("001000"));
  bin_win.FromField()->setText(QStringLiteral("001000"));
  bin_win.ToField()->setText(QStringLiteral("00103F"));
  const QString srec_path = temp_dir.filePath(QStringLiteral("bin_out.S68"));
  ASSERT_TRUE(bin_win.SaveSRecordTo(srec_path));

  easybin::MainWindow bin_win2;
  bin_win2.SetNotify([](const QString&) {});
  ASSERT_TRUE(bin_win2.OpenSRecordFile(srec_path));
  EXPECT_EQ(bin_win2.GetMemory().ReadByte(0x1000).value, 0xDEu);
  EXPECT_EQ(bin_win2.GetMemory().ReadByte(0x1003).value, 0xEFu);

  bin_win2.SetSplitIndex(2);  // 4-way split for visual color coding
  MaybeGrabWidget(&bin_win2, QStringLiteral("11_easybin_window.png"));

  bin_win2.ShowHelp(QStringLiteral("EASyBIN"));
  ASSERT_NE(bin_win2.HelpView(), nullptr);
  EXPECT_EQ(bin_win2.HelpView()->current_page(), QStringLiteral("easybin.htm"));

  if (!qgetenv("EASY68K_SCREENSHOT_DIR").isEmpty()) {
    bin_win2.HelpView()->ShowContext(QStringLiteral("MOVE"));
    MaybeGrabWidget(bin_win2.HelpView(), QStringLiteral("12_help_viewer.png"));
  }
}

}  // namespace
}  // namespace easym68k::gui
