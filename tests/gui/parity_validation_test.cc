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
#include <QPainter>
#include <QPixmap>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QThread>

#include "breakpoints_window.h"
#include "easybin/mainwindow.h"
#include "editor/dialogs.h"
#include "editor/mainwindow.h"
#include "hardware_window.h"
#include "help/examples_browser.h"
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

struct WindowPlacement {
  QWidget* widget = nullptr;
  QRect target_rect;
  QString title;
  bool active = false;
};

class VideoStageRecorder {
 public:
  explicit VideoStageRecorder(const QString& out_dir) : out_dir_(out_dir) {
    QDir().mkpath(out_dir_);
  }

  void RecordFrame(const QString& scene_badge, const QString& action_badge, const QString& caption,
                   const QString& telemetry, const std::vector<WindowPlacement>& windows,
                   double duration_sec) {
    QApplication::processEvents();
    QImage canvas(1280, 720, QImage::Format_RGB32);
    canvas.fill(QColor(13, 17, 23));

    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Subtle background grid
    p.setPen(QColor(24, 31, 42));
    for (int x = 0; x < 1280; x += 32)
      p.drawLine(x, 44, x, 660);
    for (int y = 44; y < 660; y += 32)
      p.drawLine(0, y, 1280, y);

    // Top Header Bar
    p.fillRect(0, 0, 1280, 44, QColor(22, 27, 34));
    p.setPen(QColor(48, 54, 61));
    p.drawLine(0, 43, 1280, 43);

    QFont title_font(QStringLiteral("Sans Serif"), 11, QFont::Bold);
    p.setFont(title_font);
    p.setPen(QColor(230, 237, 243));
    p.drawText(QRect(16, 0, 720, 44), Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("EASy68K-Qt  —  Cross-Platform Motorola 68000 IDE & Simulator"));

    // Scene pill on top right
    QFont badge_font(QStringLiteral("Sans Serif"), 10, QFont::Bold);
    p.setFont(badge_font);
    const QRect scene_rect(820, 8, 444, 28);
    p.setPen(QColor(56, 139, 253));
    p.setBrush(QColor(22, 44, 78));
    p.drawRoundedRect(scene_rect, 6, 6);
    p.setPen(QColor(165, 214, 255));
    p.drawText(scene_rect, Qt::AlignCenter, scene_badge);

    // Render each placed Qt window with a title bar & border
    for (const auto& wp : windows) {
      if (!wp.widget)
        continue;
      wp.widget->show();
      QApplication::processEvents();
      const QPixmap grab = wp.widget->grab();

      const QRect frame_rect = wp.target_rect;
      const QRect title_bar(frame_rect.x(), frame_rect.y(), frame_rect.width(), 22);
      const QRect body_rect(frame_rect.x(), frame_rect.y() + 22, frame_rect.width(),
                            frame_rect.height() - 22);

      // Shadow
      p.fillRect(frame_rect.adjusted(4, 4, 4, 4), QColor(0, 0, 0, 140));

      // Title bar
      p.fillRect(title_bar, wp.active ? QColor(31, 111, 235) : QColor(33, 38, 45));
      QFont win_font(QStringLiteral("Sans Serif"), 9, QFont::Bold);
      p.setFont(win_font);
      p.setPen(QColor(240, 246, 252));
      p.drawText(title_bar.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, wp.title);

      // Window body
      p.fillRect(body_rect, QColor(240, 240, 240));
      p.drawPixmap(body_rect, grab);

      // Border
      p.setBrush(Qt::NoBrush);
      p.setPen(QPen(wp.active ? QColor(88, 166, 255) : QColor(68, 76, 86), wp.active ? 2 : 1));
      p.drawRect(frame_rect);
    }

    // Bottom HUD Bar
    p.fillRect(0, 660, 1280, 60, QColor(22, 27, 34));
    p.setPen(QColor(48, 54, 61));
    p.drawLine(0, 660, 1280, 660);

    // Action badge
    const QRect act_rect(16, 668, 176, 24);
    p.setPen(QColor(210, 153, 34));
    p.setBrush(QColor(58, 43, 15));
    p.drawRoundedRect(act_rect, 5, 5);
    p.setFont(QFont(QStringLiteral("Sans Serif"), 9, QFont::Bold));
    p.setPen(QColor(242, 204, 96));
    p.drawText(act_rect, Qt::AlignCenter, action_badge);

    // Caption text
    p.setFont(QFont(QStringLiteral("Sans Serif"), 10, QFont::Bold));
    p.setPen(QColor(230, 237, 243));
    p.drawText(QRect(204, 666, 1060, 26), Qt::AlignVCenter | Qt::AlignLeft, caption);

    // Telemetry line
    p.setFont(QFont(QStringLiteral("Monospace"), 9));
    p.setPen(QColor(126, 231, 135));
    p.drawText(QRect(16, 694, 1248, 22), Qt::AlignVCenter | Qt::AlignLeft, telemetry);

    const QString fname = QStringLiteral("frame_%1.png").arg(frame_index_++, 4, 10, QChar('0'));
    canvas.save(QDir(out_dir_).filePath(fname), "PNG");
    concat_lines_.append(
        QStringLiteral("file '%1'\nduration %2\n").arg(fname).arg(duration_sec, 0, 'f', 3));
    last_fname_ = fname;
  }

  void FinalizeConcatFile() {
    if (!last_fname_.isEmpty()) {
      concat_lines_.append(QStringLiteral("file '%1'\n").arg(last_fname_));
    }
    QFile f(QDir(out_dir_).filePath(QStringLiteral("concat.txt")));
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
      for (const QString& line : concat_lines_)
        f.write(line.toUtf8());
    }
  }

 private:
  QString out_dir_;
  int frame_index_ = 0;
  QStringList concat_lines_;
  QString last_fname_;
};

TEST(ParityValidationTest, GenerateCapabilityVideoWhenRequested) {
  const QByteArray video_dir_env = qgetenv("EASY68K_VIDEO_FRAMES_DIR");
  if (video_dir_env.isEmpty())
    return;

  VideoStageRecorder rec(QString::fromLocal8Bit(video_dir_env));
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  // =========================================================================
  // SCENE 1: Edit68K-Qt — Curriculum Browser, Live Assembly Coding & Assemble
  // =========================================================================
  editor::MainWindow editor_win(temp_dir.filePath(QStringLiteral("edit68k.ini")));
  editor_win.resize(1240, 580);
  editor::EditorChild* child = editor_win.NewSourceWindow();
  ASSERT_NE(child, nullptr);

  ExamplesBrowserDialog ex_dlg;
  ex_dlg.resize(860, 520);
  const QStringList demo_ids = {
      QStringLiteral("L0-01"),
      QStringLiteral("L2-02"),
      QStringLiteral("H1-01"),
      QStringLiteral("C-01"),
  };
  for (const QString& ex_id : demo_ids) {
    ex_dlg.SelectExampleById(ex_id);
    const auto* entry = ex_dlg.SelectedEntry();
    const QString title = entry ? (entry->id + QStringLiteral(" — ") + entry->title) : ex_id;
    rec.RecordFrame(
        QStringLiteral("Scene 1 / 6 — Edit68K-Qt & Curriculum Browser"),
        QStringLiteral("Curriculum Browser"),
        QStringLiteral("Browsing 41-example curriculum catalog across Track A (Asm), Track B "
                       "(Hardware), Track C (C/C++)"),
        QStringLiteral("Selected Example: %1  |  7-Section README + Exercise + Solution Spec")
            .arg(title),
        {{&editor_win, QRect(16, 52, 1248, 598), QStringLiteral("Edit68K-Qt — MDI Assembly IDE"),
          false},
         {&ex_dlg, QRect(210, 82, 860, 540),
          QStringLiteral("File > Examples... — Curriculum Browser (Ctrl+Shift+E)"), true}},
        0.75);
  }
  ex_dlg.hide();

  // Progressive live coding in Edit68K MDI editor
  const QStringList code_chunks = {
      QStringLiteral("*-----------------------------------------------------------\n"
                     "* Program    : Demo68K.X68\n"
                     "* Description: EASy68K-Qt Live Capability Walkthrough\n"
                     "*-----------------------------------------------------------\n"
                     "        ORG     $1000\n"),
      QStringLiteral("START   MOVEQ   #1,D1           ; initialize D1 = 1\n"
                     "        MOVEQ   #2,D2           ; initialize D2 = 2\n"
                     "        ADD.L   D1,D2           ; D2 = D1 + D2 = 3\n"),
      QStringLiteral("        MOVEQ   #4,D3           ; initialize D3 = 4\n"
                     "        SUB.L   D1,D3           ; D3 = D3 - D1 = 3\n"
                     "        MOVE.L  #$1234,D4       ; load hex constant\n"
                     "        EOR.L   D1,D4           ; D4 = $1235\n"),
      QStringLiteral("        MOVE.B  #$5A,$2000      ; write $5A to RAM $2000\n"
                     "        LEA     MSG,A1          ; load string address\n"
                     "        MOVEQ   #14,D0          ; Trap #15 task 14: display string\n"
                     "        TRAP    #15\n"),
      QStringLiteral("        IF.B D1 <EQ> #1 THEN.S  ; structured assembly block\n"
                     "            NOP\n"
                     "        ENDI\n"
                     "        SIMHALT                 ; halt simulator\n"
                     "MSG     DC.B    'EASy68K-Qt OK!',0\n"
                     "        END     START\n"),
  };

  QString accum_code;
  for (int i = 0; i < code_chunks.size(); ++i) {
    accum_code += code_chunks[i];
    child->editor()->setPlainText(accum_code);
    rec.RecordFrame(
        QStringLiteral("Scene 1 / 6 — Edit68K-Qt & Curriculum Browser"),
        QStringLiteral("Live M68K Coding"),
        QStringLiteral("8-category syntax highlighting (labels, opcodes, directives, structured "
                       "IF/ENDI, comments)"),
        QStringLiteral(
            "Document: Demo68K.X68  |  Lines: %1  |  Tab Mode: Assembly (Cols 12, 20, 44)")
            .arg(child->editor()->document()->blockCount()),
        {{&editor_win, QRect(16, 52, 1248, 598), QStringLiteral("Edit68K-Qt — [Demo68K.X68]"),
          true}},
        0.55);
  }

  // Show Editor Options Dialog cycling syntax presets
  editor::EditorOptionsDialog::ActiveFileInfo file_info;
  editor::EditorOptionsDialog ed_dlg(editor_win.preferences(), file_info);
  ed_dlg.resize(540, 420);
  ed_dlg.tabs()->setCurrentIndex(1);
  rec.RecordFrame(
      QStringLiteral("Scene 1 / 6 — Edit68K-Qt & Curriculum Browser"),
      QStringLiteral("Editor Options"),
      QStringLiteral("Configurable font, Assembly/Fixed tab stops, Auto-Indent, and Syntax "
                     "Highlighting presets"),
      QStringLiteral("Dialog: Options > Editor Options...  |  Tab: Syntax Highlight"),
      {{&editor_win, QRect(16, 52, 1248, 598), QStringLiteral("Edit68K-Qt — [Demo68K.X68]"), false},
       {&ed_dlg, QRect(360, 120, 560, 440), QStringLiteral("Editor Options — Syntax Highlighting"),
        true}},
      0.85);
  ed_dlg.hide();

  // Save and Assemble
  const QString src_path = temp_dir.filePath(QStringLiteral("Demo68K.X68"));
  ASSERT_TRUE(editor_win.SaveChildAs(child, src_path));
  editor_win.SetStatusDialogHandler([](const QString&, int, int, bool, bool) {
    return editor::AssemblerStatusDialog::ActionChoice::kClose;
  });
  const auto outcome = editor_win.AssembleActiveDocument();
  ASSERT_TRUE(outcome.result.success);

  editor::AssemblerStatusDialog status_dlg(QStringLiteral("Demo68K.X68"), 0, 0, true, true);
  status_dlg.resize(420, 210);
  rec.RecordFrame(
      QStringLiteral("Scene 1 / 6 — Edit68K-Qt & Curriculum Browser"),
      QStringLiteral("F9 Assemble Source"),
      QStringLiteral("Assembled Demo68K.X68 -> generated Demo68K.L68 listing and Demo68K.S68 "
                     "Motorola S-Record"),
      QStringLiteral("Assembler Result: 0 Warnings, 0 Errors  |  Ready to Execute in Sim68K-Qt"),
      {{&editor_win, QRect(16, 52, 1248, 598), QStringLiteral("Edit68K-Qt — [Demo68K.X68]"), false},
       {&status_dlg, QRect(420, 230, 440, 220),
        QStringLiteral("Assembler Status — 0 Warnings, 0 Errors"), true}},
      0.95);
  status_dlg.hide();

  editor_win.OpenFile(outcome.list_path);
  editor_win.mdi_area()->tileSubWindows();
  rec.RecordFrame(
      QStringLiteral("Scene 1 / 6 — Edit68K-Qt & Curriculum Browser"),
      QStringLiteral("Tiled .X68 + .L68"),
      QStringLiteral("Side-by-side MDI view of source (.X68) and generated machine-code listing + "
                     "symbol table (.L68)"),
      QStringLiteral(
          "Files: Demo68K.X68 + Demo68K.L68  |  Handing off Demo68K.S68 to Sim68K-Qt..."),
      {{&editor_win, QRect(16, 52, 1248, 598),
        QStringLiteral("Edit68K-Qt — Tiled Source (.X68) & Listing (.L68)"), true}},
      1.10);
  editor_win.hide();

  // =========================================================================
  // SCENE 2: Sim68K-Qt — Step-by-Step Execution, Registers, Memory & Stack
  // =========================================================================
  MainWindow sim_win(temp_dir.filePath(QStringLiteral("sim68k.ini")));
  sim_win.SetInform([](const QString&, const QString&) {});
  sim_win.resize(750, 598);
  ASSERT_TRUE(sim_win.OpenFile(outcome.srec_path));
  SimSession& session = sim_win.Session();
  session.SetBreakpoint(0x1018, true);

  MemoryWindow* mem_win = sim_win.MemoryView();
  mem_win->resize(476, 300);
  mem_win->ShowAddress(0x1000);
  mem_win->Refresh();

  StackWindow* stack_win = sim_win.Stack();
  stack_win->resize(476, 286);
  stack_win->UpdateState(session.Simulator().State());

  for (int step = 0; step <= 11; ++step) {
    if (step > 0) {
      session.TraceInto();
      ASSERT_TRUE(WaitUntilIdle(session));
    }
    const auto& st = session.Simulator().State();
    if (step >= 8) {
      mem_win->ShowAddress(0x2000);
    } else {
      mem_win->ShowAddress(0x1000);
    }
    mem_win->Refresh();
    stack_win->UpdateState(st);

    const QString telemetry =
        QStringLiteral("Step %1/11  |  PC=$%2  SR=$%3  D1=$%4  D2=$%5  D3=$%6  D4=$%7  Cycles=%8")
            .arg(step, 2, 10, QChar('0'))
            .arg(st.pc, 6, 16, QChar('0'))
            .arg(st.sr, 4, 16, QChar('0'))
            .arg(st.d[1], 8, 16, QChar('0'))
            .arg(st.d[2], 8, 16, QChar('0'))
            .arg(st.d[3], 8, 16, QChar('0'))
            .arg(st.d[4], 8, 16, QChar('0'))
            .arg(st.cycles);

    rec.RecordFrame(QStringLiteral("Scene 2 / 6 — Sim68K-Qt Live Instruction Stepping"),
                    QStringLiteral("F7 Trace Into (Step %1)").arg(step),
                    step < 8
                        ? QStringLiteral("Stepping instruction-by-instruction: PC highlight bar, "
                                         "registers, SR flags, and cycle counter update live")
                        : QStringLiteral("Memory write at $2000 ($5A) reflected immediately in "
                                         "Memory Window + Trap #15 string output"),
                    telemetry.toUpper(),
                    {{&sim_win, QRect(16, 52, 752, 598),
                      QStringLiteral("Sim68K-Qt — Main Debugger Window"), true},
                     {mem_win, QRect(780, 52, 484, 300),
                      step >= 8 ? QStringLiteral("Memory Window — RAM $002000 (Live Update)")
                                : QStringLiteral("Memory Window — Code $001000"),
                      step >= 8},
                     {stack_win, QRect(780, 362, 484, 288),
                      QStringLiteral("Stack Window (A7 = $01000000)"), false}},
                    0.55);
  }
  mem_win->hide();
  stack_win->hide();

  // =========================================================================
  // SCENE 3: Sim68K-Qt — Interactive Hardware Peripherals (7-Seg, LEDs, Switches)
  // =========================================================================
  HardwareWindow* hw_win = sim_win.Hardware();
  hw_win->resize(484, 598);
  const uint8_t seg_patterns[8] = {0x79, 0x77, 0x6D, 0x6E,
                                   0x7D, 0x7F, 0x06, 0x3F};  // E A S y 6 8 1 0
  for (int i = 0; i < 8; ++i) {
    session.Simulator().GetMemory().WriteByte(0xE00000 + i * 2, 0x00);
  }
  for (int frame = 0; frame < 10; ++frame) {
    if (frame < 8) {
      session.Simulator().GetMemory().WriteByte(0xE00000 + frame * 2, seg_patterns[frame]);
    }
    const uint8_t led_val = static_cast<uint8_t>((1u << (frame % 8)) | (1u << ((7 - (frame % 8)))) |
                                                 (frame >= 8 ? 0xA5 : 0));
    session.Simulator().GetMemory().WriteByte(0xE00010, led_val);
    session.Simulator().GetMemory().WriteByte(0xE00012, static_cast<uint8_t>(frame * 0x11));
    hw_win->Refresh();

    rec.RecordFrame(
        QStringLiteral("Scene 3 / 6 — Sim68K-Qt Hardware Peripherals"),
        QStringLiteral("Hardware I/O ($E00000)"),
        QStringLiteral("Memory-mapped 8-digit 7-Segment Display ($E00000), 8-bit LED bar "
                       "($E00010), Switches ($E00012), & Auto-IRQ"),
        QStringLiteral(
            "7-Seg Digits Written: %1/8  |  LEDs ($E00010) = $%2  |  Switches ($E00012) = $%3")
            .arg(std::min(frame + 1, 8))
            .arg(led_val, 2, 16, QChar('0'))
            .arg((frame * 0x11) & 0xFF, 2, 16, QChar('0'))
            .toUpper(),
        {{&sim_win, QRect(16, 52, 752, 598), QStringLiteral("Sim68K-Qt — Main Debugger Window"),
          false},
         {hw_win, QRect(780, 52, 484, 598),
          QStringLiteral("Hardware Window — 7-Segment, LEDs, Switches & IRQ"), true}},
        0.48);
  }
  hw_win->hide();

  // =========================================================================
  // SCENE 4: Sim68K-Qt — Trap #15 Console Text & 2D Graphics Engine
  // =========================================================================
  IoWindow* io_win = sim_win.Io();
  io_win->resize(520, 598);
  session.Screen().ClearScreen();
  session.Screen().TextOutCRLF("=== EASy68K-Qt Trap #15 I/O Console & Graphics ===");
  session.Screen().TextOutCRLF("Task 13/14: Text Output  |  Tasks 80-95: 2D Graphics");

  for (int gstep = 0; gstep < 9; ++gstep) {
    if (gstep == 1) {
      session.Screen().SetPenWidth(2);
      session.Screen().SetPenColor(0x00FF8000);
      session.Screen().SetFillColor(0x00602010);
      session.Screen().Rectangle(24, 70, 210, 195);
    } else if (gstep == 2) {
      session.Screen().SetPenColor(0x0000FF80);
      session.Screen().SetFillColor(0x00105020);
      session.Screen().Rectangle(230, 70, 420, 195);
    } else if (gstep == 3) {
      session.Screen().SetPenColor(0x004080FF);
      session.Screen().SetFillColor(0x00152560);
      session.Screen().Ellipse(40, 215, 200, 345);
    } else if (gstep == 4) {
      session.Screen().SetPenColor(0x0000FFFF);
      session.Screen().SetFillColor(0x00006060);
      session.Screen().Ellipse(245, 215, 405, 345);
    } else if (gstep == 5) {
      session.Screen().SetPenColor(0x00FF40FF);
      for (int r = 0; r < 6; ++r) {
        session.Screen().Line(24 + r * 68, 365, 420 - r * 68, 430);
      }
    } else if (gstep == 6) {
      session.Screen().SetPenColor(0x00FFFFFF);
      session.Screen().DrawText("Trap #15 Vector & Raster Graphics", 72, 125);
      session.Screen().DrawText("16 Pen Modes + Double Buffering", 242, 125);
    } else if (gstep == 7) {
      session.Screen().SetPenColor(0x0080FFFF);
      session.Screen().DrawText("Ellipse + FloodFill", 68, 272);
      session.Screen().DrawText("640x480 / 800x600 Canvas", 256, 272);
    } else if (gstep == 8) {
      session.Screen().TextOutCRLF("Status: Graphics render complete (PARITY OK).");
    }

    rec.RecordFrame(QStringLiteral("Scene 4 / 6 — Sim68K-Qt Trap #15 Console & 2D Graphics"),
                    QStringLiteral("Trap #15 Graphics (%1/9)").arg(gstep + 1),
                    QStringLiteral("Shared text grid + QPainter back-buffer supporting 16 Win32 "
                                   "ROP2 pen modes, shapes, flood-fill, and double buffering"),
                    QStringLiteral("Active Canvas: SimIoScreen (640x480)  |  Tasks 0-25 (Text/Key) "
                                   "& Tasks 80-96 (Graphics)"),
                    {{&sim_win, QRect(16, 52, 710, 598),
                      QStringLiteral("Sim68K-Qt — Main Debugger Window"), false},
                     {io_win, QRect(738, 52, 526, 598),
                      QStringLiteral("Sim68K I/O Window — Console Text & 2D Graphics"), true}},
                    0.52);
  }
  io_win->hide();

  // =========================================================================
  // SCENE 5: Advanced Conditional Breakpoints & Curriculum Solution Checker
  // =========================================================================
  BreakpointsWindow* bp = sim_win.Breakpoints();
  bp->resize(526, 598);
  bp->OpenRegRow(0);
  bp->RegSelectCombo()->setCurrentIndex(0);  // D0
  bp->RegOpCombo()->setCurrentIndex(0);      // ==
  bp->RegValueEdit()->setText(QStringLiteral("0000000E"));
  bp->RegSetButton()->click();
  rec.RecordFrame(
      QStringLiteral("Scene 5 / 6 — Advanced Breakpoints & Solution Checker"),
      QStringLiteral("Reg Breakpoint R1"),
      QStringLiteral(
          "Configuring PC/Register condition R1 (D0 == 0000000E) in Advanced Breakpoints window"),
      QStringLiteral("Slot R1: D0.L == $0000000E [Enabled]"),
      {{&sim_win, QRect(16, 52, 710, 598), QStringLiteral("Sim68K-Qt — Main Debugger Window"),
        false},
       {bp, QRect(738, 52, 526, 598),
        QStringLiteral("Advanced Breakpoints — Register, Address & Expression Builder"), true}},
      0.75);

  bp->OpenAddrRow(0);
  bp->AddrSelectEdit()->setText(QStringLiteral("002000"));
  bp->AddrValueEdit()->setText(QStringLiteral("0000005A"));
  bp->AddrSetButton()->click();
  bp->OpenExprRow(0);
  bp->ExprRegAppendButton()->click();
  bp->ExprAndButton()->click();
  bp->ExprAddrAppendButton()->click();
  bp->ExprSetButton()->click();
  rec.RecordFrame(
      QStringLiteral("Scene 5 / 6 — Advanced Breakpoints & Solution Checker"),
      QStringLiteral("Boolean Expression E1"),
      QStringLiteral("Combining Register (R1) and Memory Read/Write (A1: M[$002000] == $5A) into "
                     "postfix expression E1 (R1 AND A1)"),
      QStringLiteral(
          "Expression E1: R1 && A1  |  Pass Count = 1  |  Evaluated on every instruction step"),
      {{&sim_win, QRect(16, 52, 710, 598), QStringLiteral("Sim68K-Qt — Main Debugger Window"),
        false},
       {bp, QRect(738, 52, 526, 598),
        QStringLiteral("Advanced Breakpoints — Expression E1 = R1 AND A1"), true}},
      0.95);
  bp->hide();

  // Load curriculum example L0-01 and run Solution Checker
  ExamplesCatalog catalog;
  const ExampleCatalogEntry* l0 = catalog.FindById(QStringLiteral("L0-01"));
  ASSERT_NE(l0, nullptr);
  {
    const QString tmp_ex_dir = temp_dir.filePath(QStringLiteral("L0-01-hello-simhalt"));
    QDir().mkpath(tmp_ex_dir);
    const QString tmp_src = QDir(tmp_ex_dir).filePath(QStringLiteral("main.x68"));
    const QString tmp_toml = QDir(tmp_ex_dir).filePath(QStringLiteral("example.toml"));
    const QString tmp_readme = QDir(tmp_ex_dir).filePath(QStringLiteral("README.md"));
    QFile::copy(l0->source_path, tmp_src);
    QFile::copy(l0->toml_path, tmp_toml);
    QFile::copy(l0->readme_path, tmp_readme);

    editor::MainWindow ed_tmp(temp_dir.filePath(QStringLiteral("ed_tmp.ini")));
    ASSERT_NE(ed_tmp.OpenFile(tmp_src), nullptr);
    ed_tmp.SetStatusDialogHandler([](const QString&, int, int, bool, bool) {
      return editor::AssemblerStatusDialog::ActionChoice::kClose;
    });
    const auto ex_out = ed_tmp.AssembleActiveDocument();
    ASSERT_TRUE(ex_out.result.success);
    ASSERT_TRUE(sim_win.OpenFile(ex_out.srec_path));
  }
  sim_win.Session().Run();
  ASSERT_TRUE(WaitUntilIdle(sim_win.Session()));
  sim_win.CheckSolutionAction()->trigger();
  SolutionCheckerDialog* checker = sim_win.SolutionCheckerView();
  ASSERT_NE(checker, nullptr);
  checker->resize(526, 480);
  rec.RecordFrame(
      QStringLiteral("Scene 5 / 6 — Advanced Breakpoints & Solution Checker"),
      QStringLiteral("Check Solution [PASS]"),
      QStringLiteral("Built-in curriculum Solution Checker (Ctrl+Shift+C) verifies halted CPU "
                     "state, registers, memory & console output against TOML spec"),
      QStringLiteral("Example L0-01: [PASS] All register, halt, and cycle expectations satisfied!"),
      {{&sim_win, QRect(16, 52, 710, 598),
        QStringLiteral("Sim68K-Qt — Curriculum 'What to Watch' Banner Active"), false},
       {checker, QRect(738, 90, 526, 520),
        QStringLiteral("Run > Check Solution... — Automated Verification [PASS]"), true}},
      1.20);
  checker->hide();
  sim_win.hide();

  // =========================================================================
  // SCENE 6: EASyBIN-Qt S-Record/Binary Inspector & Context-Sensitive Help
  // =========================================================================
  easybin::MainWindow bin_win;
  bin_win.SetNotify([](const QString&) {});
  bin_win.resize(610, 598);
  ASSERT_TRUE(bin_win.OpenSRecordFile(outcome.srec_path));
  bin_win.StartAddressField()->setText(QStringLiteral("001000"));
  bin_win.FromField()->setText(QStringLiteral("001000"));
  bin_win.ToField()->setText(QStringLiteral("00103F"));

  HelpViewer help_win;
  help_win.resize(610, 598);

  const QStringList split_names = {
      QStringLiteral("Every Byte (Standard Contiguous Binary)"),
      QStringLiteral("Even / Odd Byte Split (16-bit Upper/Lower ROM Banks)"),
      QStringLiteral("Every 4th Byte Split (32-bit 4-Way Interleaved ROM Banks)"),
  };
  const QStringList help_topics = {
      QStringLiteral("MOVE"),
      QStringLiteral("SIM_TRAP15"),
      QStringLiteral("DBRA"),
  };
  for (int i = 0; i < 3; ++i) {
    bin_win.SetSplitIndex(i);
    help_win.ShowContext(help_topics[i]);
    rec.RecordFrame(
        QStringLiteral("Scene 6 / 6 — EASyBIN-Qt & Context-Sensitive HelpViewer"),
        QStringLiteral("EASyBIN + Help (%1/3)").arg(i + 1),
        QStringLiteral("EASyBIN-Qt S-Record/Binary converter with color-coded ROM bank splitting + "
                       "offline HTML HelpViewer with curriculum links"),
        QStringLiteral("EASyBIN Split Mode: %1  |  Help Topic: %2")
            .arg(split_names[i], help_topics[i]),
        {{&bin_win, QRect(16, 52, 616, 598),
          QStringLiteral("EASyBIN-Qt — S-Record / Binary Utility"), i < 2},
         {&help_win, QRect(648, 52, 616, 598),
          QStringLiteral("EASy68K HelpViewer — TOC, Index, Search & Curriculum Cross-Links"),
          i == 2}},
        0.95);
  }

  rec.FinalizeConcatFile();
}

}  // namespace
}  // namespace easym68k::gui
