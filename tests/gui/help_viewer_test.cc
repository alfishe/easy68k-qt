// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "help/help_viewer.h"

#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QUrl>

#include <gtest/gtest.h>

#include "easybin/mainwindow.h"
#include "mainwindow.h"

namespace easym68k::gui {
namespace {

TEST(HelpViewerTest, LoadsTableOfContentsAndIndex) {
  HelpViewer viewer;
  ASSERT_FALSE(viewer.help_dir().isEmpty());
  EXPECT_EQ(viewer.current_page().toLower(), QStringLiteral("main.htm"));
  EXPECT_TRUE(viewer.current_plain_text().contains(
      QStringLiteral("EASy68K"), Qt::CaseInsensitive));

  ASSERT_NE(viewer.toc_tree(), nullptr);
  EXPECT_GE(viewer.toc_tree()->topLevelItemCount(), 8);

  ASSERT_NE(viewer.index_list(), nullptr);
  EXPECT_GT(viewer.index_list()->count(), 100);

  // Find the "EASyBIN" top-level TOC item and activate it.
  QTreeWidgetItem* easybin_item = nullptr;
  for (int i = 0; i < viewer.toc_tree()->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = viewer.toc_tree()->topLevelItem(i);
    if (item->text(0).contains(QStringLiteral("EASyBIN"),
                               Qt::CaseInsensitive)) {
      easybin_item = item;
      break;
    }
  }
  ASSERT_NE(easybin_item, nullptr);
  ASSERT_EQ(easybin_item->childCount(), 2);
  emit viewer.toc_tree()->itemClicked(easybin_item->child(0), 0);
  EXPECT_EQ(viewer.current_page().toLower(), QStringLiteral("easybin.htm"));
  EXPECT_TRUE(viewer.current_plain_text().contains(
      QStringLiteral("Binary"), Qt::CaseInsensitive));
}

TEST(HelpViewerTest, IndexFilteringAndNavigation) {
  HelpViewer viewer;
  viewer.index_filter()->setText(QStringLiteral("MOVEM"));
  ASSERT_GT(viewer.index_list()->count(), 0);
  emit viewer.index_filter()->returnPressed();
  EXPECT_EQ(viewer.current_page().toLower(),
            QStringLiteral("reference/68ks4f.htm"));
  EXPECT_TRUE(viewer.current_plain_text().contains(
      QStringLiteral("MOVEM"), Qt::CaseInsensitive));
}

TEST(HelpViewerTest, ContextResolutionCoversAllAppsInstructionsAndDirectives) {
  HelpViewer viewer;

  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("INTRO")).toLower(),
            QStringLiteral("main.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("MAIN")).toLower(),
            QStringLiteral("main.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("UNKNOWN_TOKEN")).toLower(),
            QStringLiteral("main.htm"));

  // Simulator contexts (including original lookup bug fixes).
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("SIM_BASIC")).toLower(),
            QStringLiteral("simhelp/basicoperation.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("SIM_MEMORY")).toLower(),
            QStringLiteral("simhelp/memory.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("SIM_OPTIONS")).toLower(),
            QStringLiteral("simhelp/simoptions.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("STACK")).toLower(),
            QStringLiteral("simhelp/stack.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("HARDWARE")).toLower(),
            QStringLiteral("simhelp/hardware.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("BREAKPOINT")).toLower(),
            QStringLiteral("simhelp/advbreakpoints.htm"));

  // Editor & EASyBIN contexts.
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("EDIT_OPTIONS")).toLower(),
            QStringLiteral("options.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("EASyBIN")).toLower(),
            QStringLiteral("easybin.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("CHUCK")).toLower(),
            QStringLiteral("credits.htm"));

  // 68000 instructions, size-suffixed mnemonics, condition codes, bitfields.
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("MOVE")).toLower(),
            QStringLiteral("reference/68ks4d.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("MOVE.W")).toLower(),
            QStringLiteral("reference/68ks4d.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("MOVE USP")).toLower(),
            QStringLiteral("reference/68ks10b.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("BNE")).toLower(),
            QStringLiteral("reference/68ks9a.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("DBRA")).toLower(),
            QStringLiteral("reference/68ks9c.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("SEQ")).toLower(),
            QStringLiteral("reference/68ks9h.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("BFEXTU")).toLower(),
            QStringLiteral("reference/bfextu.htm"));

  // Directives and structured control keywords.
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("DC")).toLower(),
            QStringLiteral("directives/dc.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("MACRO")).toLower(),
            QStringLiteral("directives/macro.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("ENDM")).toLower(),
            QStringLiteral("directives/macro.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("NOMEX")).toLower(),
            QStringLiteral("directives/opt.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("IFEQ")).toLower(),
            QStringLiteral("directives/conditional.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("FOR")).toLower(),
            QStringLiteral("struccontrol/for.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("ENDF")).toLower(),
            QStringLiteral("struccontrol/for.htm"));
  EXPECT_EQ(viewer.ResolveContextPage(QStringLiteral("UNLESS")).toLower(),
            QStringLiteral("struccontrol/dbloop.htm"));
}

TEST(HelpViewerTest, QuickReferencePdfLinkInvokesPdfOpener) {
  HelpViewer viewer;
  QString opened_pdf;
  viewer.SetPdfOpener([&opened_pdf](const QString& path) { opened_pdf = path; });

  EXPECT_TRUE(viewer.ShowContext(QStringLiteral("68000")));
  EXPECT_EQ(viewer.current_page().toLower(),
            QStringLiteral("quick reference.htm"));

  emit viewer.browser()->anchorClicked(
      QUrl(QStringLiteral("EASy68KQuickRef.pdf")));
  EXPECT_TRUE(opened_pdf.endsWith(QStringLiteral("EASy68KQuickRef.pdf")));
  EXPECT_TRUE(QFileInfo::exists(opened_pdf));
}

TEST(HelpViewerTest, Sim68KAndEasyBinHelpActionsOpenViewer) {
  MainWindow sim_win{QString()};
  ASSERT_NE(sim_win.HelpAction(), nullptr);
  EXPECT_TRUE(sim_win.HelpAction()->isEnabled());
  sim_win.HelpAction()->trigger();
  ASSERT_NE(sim_win.HelpView(), nullptr);
  EXPECT_EQ(sim_win.HelpView()->current_page().toLower(),
            QStringLiteral("simhelp/basicoperation.htm"));

  sim_win.ShowHelp(QStringLiteral("SIM_MEMORY"));
  EXPECT_EQ(sim_win.HelpView()->current_page().toLower(),
            QStringLiteral("simhelp/memory.htm"));

  easym68k::easybin::MainWindow bin_win;
  ASSERT_NE(bin_win.HelpAction(), nullptr);
  bin_win.HelpAction()->trigger();
  ASSERT_NE(bin_win.HelpView(), nullptr);
  EXPECT_EQ(bin_win.HelpView()->current_page().toLower(),
            QStringLiteral("easybin.htm"));
}

}  // namespace
}  // namespace easym68k::gui
