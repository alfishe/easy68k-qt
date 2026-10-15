// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Task E9 — Shared curriculum catalog, Edit68K Examples Browser dialog, and
// Sim68K Solution Checker dialog (doc/examples/EDUCATION_STRATEGY.md §7).

#ifndef EASYM68K_GUI_HELP_EXAMPLES_BROWSER_H_
#define EASYM68K_GUI_HELP_EXAMPLES_BROWSER_H_

#include <cstdint>
#include <functional>
#include <QDialog>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

#include "easym68k/sim/cpu_state.h"
#include "easym68k/sim/memory.h"
#include "example_runner.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTextBrowser;
class QTreeWidget;
class QTreeWidgetItem;

namespace easym68k::gui {

struct ExampleCatalogEntry {
  QString id;
  QString title;
  QString track;
  int level = 0;
  int difficulty = 1;
  QStringList requires_ids;

  QString dir_path;
  QString toml_path;
  QString readme_path;
  QString source_path;
  QString exercise_toml_path;
  QString exercise_source_path;
  QString solution_source_path;
  QString naive_source_path;

  QString readme_markdown;
  QString what_to_watch;
  QStringList mnemonics_used;
  tools::ExampleSpec spec;
};

class ExamplesCatalog {
 public:
  explicit ExamplesCatalog(const QString& examples_dir = QString());

  static QString DefaultExamplesDir();

  const QString& examples_dir() const { return examples_dir_; }
  const QVector<ExampleCatalogEntry>& entries() const { return entries_; }

  const ExampleCatalogEntry* FindById(const QString& id) const;
  const ExampleCatalogEntry* FindByFilePath(const QString& file_path) const;
  QVector<const ExampleCatalogEntry*> FindByMnemonic(const QString& mnemonic) const;

  // Resolves the most specific TOML spec (`exercises/<name>.toml` or
  // `example.toml`) associated with a `.S68`/`.X68`/`.c`/`.cpp` file.
  static QString ResolveSpecPathForProgram(const QString& program_path);

  // Extracts the "Run & Observe" (or "Predict") section from a README.md.
  static QString ExtractWhatToWatch(const QString& readme_markdown);

 private:
  void ScanDirectory();

  QString examples_dir_;
  QVector<ExampleCatalogEntry> entries_;
  QHash<QString, int> id_index_;
};

// Edit68K File -> Examples... browser dialog (EDUCATION_STRATEGY.md §7).
class ExamplesBrowserDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ExamplesBrowserDialog(QWidget* parent = nullptr,
                                 const QString& examples_dir = QString());

  const ExamplesCatalog& catalog() const { return catalog_; }
  const ExampleCatalogEntry* SelectedEntry() const;

  bool SelectExampleById(const QString& id);
  void SetTrackFilter(const QString& track);
  void SetLevelFilter(int level);  // -1 for all levels
  void SetSearchFilter(const QString& text);

  void SetOpenFileCallback(
      std::function<void(const QString& source_path, const QString& readme_path)> cb) {
    open_file_cb_ = std::move(cb);
  }

  QComboBox* track_combo() const { return track_combo_; }
  QComboBox* level_combo() const { return level_combo_; }
  QLineEdit* search_edit() const { return search_edit_; }
  QTreeWidget* examples_tree() const { return tree_; }
  QTextBrowser* readme_browser() const { return readme_browser_; }
  QPushButton* open_example_button() const { return open_example_btn_; }
  QPushButton* open_exercise_button() const { return open_exercise_btn_; }
  QPushButton* open_naive_button() const { return open_naive_btn_; }

  void TriggerOpenSelectedExample();
  void TriggerOpenSelectedExercise();
  void TriggerOpenSelectedNaive();

 private:
  void BuildUi();
  void PopulateTree();
  void OnSelectionChanged();
  void UpdateReadmePreview(const ExampleCatalogEntry* entry);

  ExamplesCatalog catalog_;
  std::function<void(const QString& source_path, const QString& readme_path)> open_file_cb_;

  QComboBox* track_combo_ = nullptr;
  QComboBox* level_combo_ = nullptr;
  QLineEdit* search_edit_ = nullptr;
  QTreeWidget* tree_ = nullptr;
  QLabel* meta_label_ = nullptr;
  QTextBrowser* readme_browser_ = nullptr;
  QPushButton* open_example_btn_ = nullptr;
  QPushButton* open_exercise_btn_ = nullptr;
  QPushButton* open_naive_btn_ = nullptr;
  QPushButton* close_btn_ = nullptr;
};

struct SolutionCheckReport {
  bool valid_spec = false;
  bool passed = false;
  QString spec_id;
  QString spec_title;
  QString spec_path;
  QString what_to_watch;
  uint64_t cycles = 0;
  QStringList passed_checks;
  QStringList failures;
  QString summary;
};

// Evaluates a live Sim68K simulator state against `spec`.
SolutionCheckReport CheckSimulatorStateAgainstSpec(const tools::ExampleSpec& spec,
                                                   const sim::CpuState& state, bool halted,
                                                   const sim::Memory& memory,
                                                   const QString& console_output,
                                                   const QString& spec_path = QString(),
                                                   const QString& what_to_watch = QString());

// Runs a clean headless execution via `RunExampleFile` and formats a report.
SolutionCheckReport RunHeadlessSolutionCheck(const QString& spec_path,
                                             const QString& source_override = QString(),
                                             const QString& what_to_watch = QString());

// Sim68K File -> Check Solution... dialog (EDUCATION_STRATEGY.md §7).
class SolutionCheckerDialog : public QDialog {
  Q_OBJECT

 public:
  using LiveCheckCallback = std::function<SolutionCheckReport(const QString& spec_path)>;

  explicit SolutionCheckerDialog(QWidget* parent = nullptr,
                                 const QString& initial_spec_path = QString(),
                                 const QString& what_to_watch = QString(),
                                 LiveCheckCallback live_check_cb = {});

  void SetSpecPath(const QString& spec_path, const QString& what_to_watch = QString());
  QString spec_path() const;

  SolutionCheckReport RunLiveCheck();
  SolutionCheckReport RunCleanVerification();

  const SolutionCheckReport& last_report() const { return last_report_; }
  QLineEdit* spec_path_edit() const { return spec_edit_; }
  QLabel* status_label() const { return status_label_; }
  QTextBrowser* details_browser() const { return details_browser_; }
  QPushButton* check_live_button() const { return check_live_btn_; }
  QPushButton* run_clean_button() const { return run_clean_btn_; }

 private:
  void BuildUi();
  void RenderReport(const SolutionCheckReport& report, const QString& mode_label);
  void RenderSpecOverview();

  QString what_to_watch_;
  LiveCheckCallback live_check_cb_;
  SolutionCheckReport last_report_;

  QLineEdit* spec_edit_ = nullptr;
  QPushButton* browse_btn_ = nullptr;
  QLabel* status_label_ = nullptr;
  QTextBrowser* details_browser_ = nullptr;
  QPushButton* check_live_btn_ = nullptr;
  QPushButton* run_clean_btn_ = nullptr;
  QPushButton* close_btn_ = nullptr;
};

}  // namespace easym68k::gui

#endif  // EASYM68K_GUI_HELP_EXAMPLES_BROWSER_H_
