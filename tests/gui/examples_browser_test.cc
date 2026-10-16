// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Task E9 — Tests for the Curriculum Catalog, Edit68K Examples Browser,
// Sim68K Solution Checker & "What to Watch" hints, and HelpViewer instruction
// cross-links (doc/examples/EDUCATION_STRATEGY.md §7).

#include "help/examples_browser.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QThread>
#include <QTreeWidget>

#include "easym68k/asm/assembler.h"
#include "editor/mainwindow.h"
#include "help/help_viewer.h"
#include "mainwindow.h"

namespace easym68k::gui {
namespace {

bool WaitUntilNotRunning(SimSession& session, int timeout_ms = 3000) {
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

TEST(ExamplesBrowserTest, CatalogDiscoversAll83CurriculumExamplesAndMnemonics) {
  ExamplesCatalog catalog;
  EXPECT_GE(catalog.entries().size(), 83);

  const ExampleCatalogEntry* l0_01 = catalog.FindById(QStringLiteral("L0-01"));
  ASSERT_NE(l0_01, nullptr);
  EXPECT_EQ(l0_01->track, QStringLiteral("core"));
  EXPECT_EQ(l0_01->level, 0);
  EXPECT_FALSE(l0_01->what_to_watch.isEmpty());
  EXPECT_TRUE(l0_01->mnemonics_used.contains(QStringLiteral("NOP")));

  const ExampleCatalogEntry* o_04 = catalog.FindById(QStringLiteral("O-04"));
  ASSERT_NE(o_04, nullptr);
  EXPECT_EQ(o_04->track, QStringLiteral("optimization"));
  EXPECT_FALSE(o_04->naive_source_path.isEmpty());
  EXPECT_TRUE(o_04->spec.expect_max_cycles.has_value());

  const ExampleCatalogEntry* p_04 = catalog.FindById(QStringLiteral("P-04"));
  ASSERT_NE(p_04, nullptr);
  EXPECT_EQ(p_04->track, QStringLiteral("projects"));
  EXPECT_EQ(p_04->level, 5);

  const auto dbra_examples = catalog.FindByMnemonic(QStringLiteral("DBRA"));
  EXPECT_FALSE(dbra_examples.isEmpty());
}

TEST(ExamplesBrowserTest, Edit68KExamplesBrowserFiltersAndOpensSourceAndExercises) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const QString settings_path = temp_dir.filePath(QStringLiteral("edit68k.ini"));

  editor::MainWindow win(settings_path);
  ASSERT_NE(win.examples_action(), nullptr);
  win.examples_action()->trigger();

  ExamplesBrowserDialog* dlg = win.examples_browser();
  ASSERT_NE(dlg, nullptr);
  EXPECT_GE(dlg->examples_tree()->topLevelItemCount(), 83);

  // Filter by Track O (13 pairs)
  dlg->SetTrackFilter(QStringLiteral("optimization"));
  EXPECT_EQ(dlg->examples_tree()->topLevelItemCount(), 13);

  // Select O-04 and open its naive baseline and main example
  EXPECT_TRUE(dlg->SelectExampleById(QStringLiteral("O-04")));
  ASSERT_NE(dlg->SelectedEntry(), nullptr);
  EXPECT_EQ(dlg->SelectedEntry()->id, QStringLiteral("O-04"));
  EXPECT_TRUE(dlg->open_naive_button()->isEnabled());
  EXPECT_TRUE(dlg->open_exercise_button()->isEnabled());
  EXPECT_TRUE(dlg->readme_browser()->toPlainText().contains(QStringLiteral("DBRA")));
  MaybeGrabWidget(dlg, QStringLiteral("13_edit68k_examples_browser.png"));

  dlg->TriggerOpenSelectedNaive();
  ASSERT_NE(win.ActiveChild(), nullptr);
  EXPECT_TRUE(win.ActiveChild()->current_file().endsWith(QStringLiteral("naive.x68")));

  dlg->TriggerOpenSelectedExercise();
  ASSERT_NE(win.ActiveChild(), nullptr);
  EXPECT_TRUE(win.ActiveChild()->current_file().endsWith(QStringLiteral("exercises/ex1.x68")));

  dlg->TriggerOpenSelectedExample();
  ASSERT_NE(win.ActiveChild(), nullptr);
  EXPECT_TRUE(win.ActiveChild()->current_file().endsWith(QStringLiteral("main.x68")));
}

TEST(ExamplesBrowserTest, Sim68KShowsWhatToWatchAndChecksSolutionLiveAndClean) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  ExamplesCatalog catalog;
  const ExampleCatalogEntry* l0_01 = catalog.FindById(QStringLiteral("L0-01"));
  ASSERT_NE(l0_01, nullptr);

  // Copy L0-01 into temp_dir so assembling main.S68 does not dirty the repo.
  const QString work_dir = temp_dir.filePath(QStringLiteral("L0-01-hello-simhalt"));
  ASSERT_TRUE(QDir().mkpath(work_dir));
  ASSERT_TRUE(QFile::copy(l0_01->source_path, QDir(work_dir).filePath(QStringLiteral("main.x68"))));
  ASSERT_TRUE(
      QFile::copy(l0_01->toml_path, QDir(work_dir).filePath(QStringLiteral("example.toml"))));
  ASSERT_TRUE(
      QFile::copy(l0_01->readme_path, QDir(work_dir).filePath(QStringLiteral("README.md"))));

  const QString x68_path = QDir(work_dir).filePath(QStringLiteral("main.x68"));
  const QString s68_path = QDir(work_dir).filePath(QStringLiteral("main.S68"));

  QFile src_file(x68_path);
  ASSERT_TRUE(src_file.open(QIODevice::ReadOnly));
  asm_::Assembler assembler;
  const auto asm_res = assembler.Assemble(src_file.readAll().toStdString());
  ASSERT_TRUE(asm_res.success);
  QFile s68_file(s68_path);
  ASSERT_TRUE(s68_file.open(QIODevice::WriteOnly));
  s68_file.write(QByteArray::fromStdString(asm_res.srecord));
  s68_file.close();

  MainWindow sim_win(temp_dir.filePath(QStringLiteral("sim68k.ini")));
  ASSERT_TRUE(sim_win.OpenFile(s68_path));

  // Verify "What to watch" was detected and shown in the message pane.
  EXPECT_FALSE(sim_win.ActiveExampleSpecPath().isEmpty());
  EXPECT_FALSE(sim_win.ActiveWhatToWatch().isEmpty());
  EXPECT_TRUE(sim_win.MessageText().contains(QStringLiteral("[What to watch]")));

  // Before running: live check fails because the program has not halted yet.
  const SolutionCheckReport pre_run = sim_win.CheckCurrentSolution();
  EXPECT_TRUE(pre_run.valid_spec);
  EXPECT_FALSE(pre_run.passed);

  // Run L0-01 to completion (NOP; SIMHALT) and check again.
  sim_win.RunAction()->trigger();
  ASSERT_TRUE(WaitUntilNotRunning(sim_win.Session()));
  ASSERT_EQ(sim_win.Session().State(), SimSession::RunState::kHalted);

  sim_win.CheckSolutionAction()->trigger();
  SolutionCheckerDialog* checker = sim_win.SolutionCheckerView();
  ASSERT_NE(checker, nullptr);
  EXPECT_TRUE(checker->last_report().passed);

  // Also test clean verification button from the dialog.
  const SolutionCheckReport clean_rep = checker->RunCleanVerification();
  EXPECT_TRUE(clean_rep.passed);
  MaybeGrabWidget(checker, QStringLiteral("14_sim68k_solution_checker.png"));
}

TEST(ExamplesBrowserTest, HelpViewerCrossLinksInstructionsToCurriculumExamples) {
  HelpViewer viewer;
  ASSERT_TRUE(viewer.ShowContext(QStringLiteral("DBRA")));
  EXPECT_TRUE(viewer.current_html().contains(QStringLiteral("Related Examples in Curriculum")));
  EXPECT_TRUE(viewer.current_html().contains(QStringLiteral("example://")));
  MaybeGrabWidget(&viewer, QStringLiteral("15_help_viewer_curriculum_links.png"));

  QString activated_id;
  viewer.SetExampleActivationCallback([&activated_id](const QString& id) { activated_id = id; });

  ASSERT_TRUE(viewer.NavigateToPage(QStringLiteral("example://L2-02")));
  EXPECT_EQ(activated_id, QStringLiteral("L2-02"));
  EXPECT_TRUE(viewer.current_plain_text().contains(QStringLiteral("DBRA")));
}

}  // namespace
}  // namespace easym68k::gui
