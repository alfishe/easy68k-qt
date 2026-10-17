// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Task E9 — Shared curriculum catalog, Edit68K Examples Browser dialog, and
// Sim68K Solution Checker dialog (doc/examples/EDUCATION_STRATEGY.md §7).

#include "help/examples_browser.h"

#include <algorithm>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QSplitter>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <utility>

namespace easym68k::gui {
namespace {

constexpr int kEntryIndexRole = Qt::UserRole;

QString ReadUtf8File(const QString& path) {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
    return QString();
  return QString::fromUtf8(f.readAll());
}

QString DifficultyBullets(int difficulty) {
  const int clamped = std::clamp(difficulty, 1, 4);
  return QString(clamped, QChar(0x25CF));  // ●
}

// Extracts canonical 68000 instruction family names used in an assembly source
// file so HelpViewer can cross-link instruction pages to relevant examples.
QStringList ExtractMnemonicsFromSource(const QString& asm_text) {
  static const QSet<QString> kKnownMnemonics = {
      QStringLiteral("ABCD"),    QStringLiteral("ADD"),   QStringLiteral("ADDA"),
      QStringLiteral("ADDI"),    QStringLiteral("ADDQ"),  QStringLiteral("ADDX"),
      QStringLiteral("AND"),     QStringLiteral("ANDI"),  QStringLiteral("ASL"),
      QStringLiteral("ASR"),     QStringLiteral("BCC"),   QStringLiteral("BCHG"),
      QStringLiteral("BCLR"),    QStringLiteral("BCS"),   QStringLiteral("BEQ"),
      QStringLiteral("BGE"),     QStringLiteral("BGT"),   QStringLiteral("BHI"),
      QStringLiteral("BLE"),     QStringLiteral("BLS"),   QStringLiteral("BLT"),
      QStringLiteral("BMI"),     QStringLiteral("BNE"),   QStringLiteral("BPL"),
      QStringLiteral("BRA"),     QStringLiteral("BSET"),  QStringLiteral("BSR"),
      QStringLiteral("BTST"),    QStringLiteral("BVC"),   QStringLiteral("BVS"),
      QStringLiteral("CHK"),     QStringLiteral("CLR"),   QStringLiteral("CMP"),
      QStringLiteral("CMPA"),    QStringLiteral("CMPI"),  QStringLiteral("CMPM"),
      QStringLiteral("DBCC"),    QStringLiteral("DBEQ"),  QStringLiteral("DBF"),
      QStringLiteral("DBNE"),    QStringLiteral("DBRA"),  QStringLiteral("DIVS"),
      QStringLiteral("DIVU"),    QStringLiteral("EOR"),   QStringLiteral("EORI"),
      QStringLiteral("EXG"),     QStringLiteral("EXT"),   QStringLiteral("JMP"),
      QStringLiteral("JSR"),     QStringLiteral("LEA"),   QStringLiteral("LINK"),
      QStringLiteral("LSL"),     QStringLiteral("LSR"),   QStringLiteral("MOVE"),
      QStringLiteral("MOVEA"),   QStringLiteral("MOVEM"), QStringLiteral("MOVEP"),
      QStringLiteral("MOVEQ"),   QStringLiteral("MULS"),  QStringLiteral("MULU"),
      QStringLiteral("NBCD"),    QStringLiteral("NEG"),   QStringLiteral("NEGX"),
      QStringLiteral("NOP"),     QStringLiteral("NOT"),   QStringLiteral("OR"),
      QStringLiteral("ORI"),     QStringLiteral("PEA"),   QStringLiteral("RESET"),
      QStringLiteral("ROL"),     QStringLiteral("ROR"),   QStringLiteral("ROXL"),
      QStringLiteral("ROXR"),    QStringLiteral("RTE"),   QStringLiteral("RTR"),
      QStringLiteral("RTS"),     QStringLiteral("SBCD"),  QStringLiteral("SCC"),
      QStringLiteral("SCS"),     QStringLiteral("SEQ"),   QStringLiteral("SF"),
      QStringLiteral("SGE"),     QStringLiteral("SGT"),   QStringLiteral("SHI"),
      QStringLiteral("SIMHALT"), QStringLiteral("SLE"),   QStringLiteral("SLS"),
      QStringLiteral("SLT"),     QStringLiteral("SMI"),   QStringLiteral("SNE"),
      QStringLiteral("SPL"),     QStringLiteral("ST"),    QStringLiteral("STOP"),
      QStringLiteral("SUB"),     QStringLiteral("SUBA"),  QStringLiteral("SUBI"),
      QStringLiteral("SUBQ"),    QStringLiteral("SUBX"),  QStringLiteral("SVC"),
      QStringLiteral("SVS"),     QStringLiteral("SWAP"),  QStringLiteral("TAS"),
      QStringLiteral("TRAP"),    QStringLiteral("TRAPV"), QStringLiteral("TST"),
      QStringLiteral("UNLK"),
  };

  QSet<QString> found;
  const QStringList lines = asm_text.split(QLatin1Char('\n'));
  const QRegularExpression ws_re(QStringLiteral("\\s+"));
  for (const QString& raw_line : lines) {
    QString line = raw_line;
    if (line.trimmed().startsWith(QLatin1Char('*')) ||
        line.trimmed().startsWith(QLatin1Char(';'))) {
      continue;
    }
    const int semi = line.indexOf(QLatin1Char(';'));
    if (semi >= 0) {
      line = line.left(semi);
    }
    if (line.trimmed().isEmpty())
      continue;

    // Strip optional label in column 0.
    QString rest = line;
    if (!rest.isEmpty() && !rest[0].isSpace()) {
      const int sp = rest.indexOf(ws_re);
      if (sp < 0)
        continue;
      rest = rest.mid(sp);
    }
    rest = rest.trimmed();
    if (rest.isEmpty())
      continue;

    const QString token = rest.section(ws_re, 0, 0).section(QLatin1Char('.'), 0, 0).toUpper();
    if (kKnownMnemonics.contains(token)) {
      found.insert(token);
      // Map conditional branches/DBcc/Scc to their family help keys as well.
      if (token.startsWith(QLatin1Char('B')) && token != QStringLiteral("BRA") &&
          token != QStringLiteral("BSR") && token != QStringLiteral("BTST") &&
          token != QStringLiteral("BSET") && token != QStringLiteral("BCLR") &&
          token != QStringLiteral("BCHG")) {
        found.insert(QStringLiteral("BCC"));
      } else if (token.startsWith(QStringLiteral("DB"))) {
        found.insert(QStringLiteral("DBCC"));
      } else if (token.startsWith(QLatin1Char('S')) && token.size() <= 3 &&
                 token != QStringLiteral("SUB")) {
        found.insert(QStringLiteral("SCC"));
      }
    }
  }

  QStringList list = found.values();
  list.sort();
  return list;
}

bool LookupRegisterValue(const sim::CpuState& state, const QString& reg_name, uint32_t* out) {
  const QString upper = reg_name.trimmed().toUpper();
  if (upper.size() == 2 && upper[0] == QLatin1Char('D') && upper[1] >= QLatin1Char('0') &&
      upper[1] <= QLatin1Char('7')) {
    *out = state.d[upper[1].unicode() - '0'];
    return true;
  }
  if (upper.size() == 2 && upper[0] == QLatin1Char('A') && upper[1] >= QLatin1Char('0') &&
      upper[1] <= QLatin1Char('7')) {
    *out = state.a[upper[1].unicode() - '0'];
    return true;
  }
  if (upper == QStringLiteral("PC")) {
    *out = state.pc;
    return true;
  }
  if (upper == QStringLiteral("SR")) {
    *out = state.sr;
    return true;
  }
  if (upper == QStringLiteral("USP")) {
    *out = state.usp;
    return true;
  }
  if (upper == QStringLiteral("SSP")) {
    *out = state.ssp;
    return true;
  }
  return false;
}

}  // namespace

// ---------------------------------------------------------------------------
// ExamplesCatalog
// ---------------------------------------------------------------------------

ExamplesCatalog::ExamplesCatalog(const QString& examples_dir)
    : examples_dir_(examples_dir.isEmpty() ? DefaultExamplesDir() : examples_dir) {
  ScanDirectory();
}

QString ExamplesCatalog::DefaultExamplesDir() {
  const QString env_dir = qEnvironmentVariable("EASY68K_EXAMPLES_DIR");
  if (!env_dir.isEmpty() && QDir(env_dir).exists()) {
    return QDir(env_dir).absolutePath();
  }

  if (QCoreApplication::instance() != nullptr) {
    const QString app_dir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(app_dir).filePath(QStringLiteral("examples")),
        QDir(app_dir).filePath(QStringLiteral("../examples")),
        QDir(app_dir).filePath(QStringLiteral("../../examples")),
        QDir(app_dir).filePath(QStringLiteral("../share/easy68k/examples")),
        QDir(app_dir).filePath(QStringLiteral("../Resources/data/examples")),
    };
    for (const QString& candidate : candidates) {
      if (QFileInfo::exists(QDir(candidate).filePath(QStringLiteral("README.md")))) {
        return QDir(candidate).absolutePath();
      }
    }
  }

#ifdef EASY68K_EXAMPLES_DIR
  const QString compiled_dir = QStringLiteral(EASY68K_EXAMPLES_DIR);
  if (QDir(compiled_dir).exists()) {
    return QDir(compiled_dir).absolutePath();
  }
#endif

  return QString();
}

QString ExamplesCatalog::ExtractWhatToWatch(const QString& readme_markdown) {
  if (readme_markdown.isEmpty())
    return QString();

  const QStringList lines = readme_markdown.split(QLatin1Char('\n'));
  QStringList watch_lines;
  bool in_section = false;
  for (const QString& raw : lines) {
    const QString trimmed = raw.trimmed();
    if (trimmed.startsWith(QStringLiteral("## "))) {
      const QString heading = trimmed.mid(3).trimmed().toLower();
      if (heading.contains(QStringLiteral("run & observe")) ||
          heading.contains(QStringLiteral("what to watch"))) {
        in_section = true;
        continue;
      }
      if (in_section) {
        break;
      }
    } else if (in_section && !trimmed.isEmpty()) {
      watch_lines.push_back(trimmed);
    }
  }
  return watch_lines.join(QStringLiteral(" "));
}

void ExamplesCatalog::ScanDirectory() {
  entries_.clear();
  id_index_.clear();
  if (examples_dir_.isEmpty() || !QDir(examples_dir_).exists())
    return;

  QStringList toml_files;
  QDirIterator it(examples_dir_, QStringList{QStringLiteral("example.toml")}, QDir::Files,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString path = it.next();
    if (path.contains(QStringLiteral("/_template/")) ||
        path.contains(QStringLiteral("\\_template\\"))) {
      continue;
    }
    toml_files.push_back(path);
  }
  toml_files.sort();

  for (const QString& toml_path : toml_files) {
    tools::ExampleSpec spec;
    std::string err;
    if (!tools::LoadExampleSpecFile(toml_path.toStdString(), &spec, &err))
      continue;

    const QFileInfo toml_info(toml_path);
    const QDir dir = toml_info.dir();

    ExampleCatalogEntry entry;
    entry.id = QString::fromStdString(spec.id);
    entry.title = QString::fromStdString(spec.title);
    entry.track = QString::fromStdString(spec.track);
    entry.level = spec.level;
    entry.difficulty = spec.difficulty;
    for (const std::string& req : spec.requires_ids) {
      entry.requires_ids.push_back(QString::fromStdString(req));
    }
    entry.dir_path = dir.absolutePath();
    entry.toml_path = toml_info.absoluteFilePath();
    entry.readme_path = dir.filePath(QStringLiteral("README.md"));
    // Prefer main.x68 when present so Edit68K opens 68000 source directly,
    // falling back to spec.source (e.g. main.c / main.cpp).
    const QString main_x68 = dir.filePath(QStringLiteral("main.x68"));
    if (QFileInfo::exists(main_x68)) {
      entry.source_path = main_x68;
    } else {
      entry.source_path = dir.filePath(QString::fromStdString(spec.source));
    }

    const QString ex_toml = dir.filePath(QStringLiteral("exercises/ex1.toml"));
    if (QFileInfo::exists(ex_toml)) {
      entry.exercise_toml_path = ex_toml;
    }
    const QString ex_x68 = dir.filePath(QStringLiteral("exercises/ex1.x68"));
    if (QFileInfo::exists(ex_x68)) {
      entry.exercise_source_path = ex_x68;
    }
    const QString sol_x68 = dir.filePath(QStringLiteral("solutions/ex1.x68"));
    if (QFileInfo::exists(sol_x68)) {
      entry.solution_source_path = sol_x68;
    }
    const QString naive_x68 = dir.filePath(QStringLiteral("naive.x68"));
    if (QFileInfo::exists(naive_x68)) {
      entry.naive_source_path = naive_x68;
    }

    entry.readme_markdown = ReadUtf8File(entry.readme_path);
    entry.what_to_watch = ExtractWhatToWatch(entry.readme_markdown);
    entry.mnemonics_used = ExtractMnemonicsFromSource(ReadUtf8File(main_x68));
    entry.spec = std::move(spec);

    entries_.push_back(std::move(entry));
  }

  std::sort(entries_.begin(), entries_.end(),
            [](const ExampleCatalogEntry& a, const ExampleCatalogEntry& b) {
              if (a.level != b.level)
                return a.level < b.level;
              return a.id < b.id;
            });

  for (int i = 0; i < entries_.size(); ++i) {
    id_index_.insert(entries_[i].id.toUpper(), i);
  }
}

const ExampleCatalogEntry* ExamplesCatalog::FindById(const QString& id) const {
  const auto it = id_index_.find(id.trimmed().toUpper());
  if (it == id_index_.end())
    return nullptr;
  return &entries_[*it];
}

const ExampleCatalogEntry* ExamplesCatalog::FindByFilePath(const QString& file_path) const {
  if (file_path.isEmpty())
    return nullptr;
  const QString abs = QFileInfo(file_path).canonicalFilePath().isEmpty()
                          ? QFileInfo(file_path).absoluteFilePath()
                          : QFileInfo(file_path).canonicalFilePath();
  for (const ExampleCatalogEntry& entry : entries_) {
    const QString entry_dir = QFileInfo(entry.dir_path).canonicalFilePath().isEmpty()
                                  ? QFileInfo(entry.dir_path).absoluteFilePath()
                                  : QFileInfo(entry.dir_path).canonicalFilePath();
    if (abs.startsWith(entry_dir + QLatin1Char('/')) || abs == entry_dir) {
      return &entry;
    }
  }
  return nullptr;
}

QVector<const ExampleCatalogEntry*> ExamplesCatalog::FindByMnemonic(const QString& mnemonic) const {
  const QString target = mnemonic.trimmed().toUpper();
  QVector<const ExampleCatalogEntry*> matches;
  if (target.isEmpty())
    return matches;
  for (const ExampleCatalogEntry& entry : entries_) {
    if (entry.mnemonics_used.contains(target)) {
      matches.push_back(&entry);
    }
  }
  return matches;
}

QString ExamplesCatalog::ResolveSpecPathForProgram(const QString& program_path) {
  if (program_path.isEmpty())
    return QString();
  const QFileInfo info(program_path);
  const QDir dir = info.dir();
  const QString stem = info.completeBaseName();

  // 1. Same-stem .toml in the same directory (e.g. exercises/ex1.S68 -> exercises/ex1.toml)
  const QString same_stem_toml = dir.filePath(stem + QStringLiteral(".toml"));
  if (QFileInfo::exists(same_stem_toml)) {
    return QFileInfo(same_stem_toml).absoluteFilePath();
  }

  // 2. If inside solutions/<name>.S68, check ../exercises/<name>.toml
  if (dir.dirName().compare(QStringLiteral("solutions"), Qt::CaseInsensitive) == 0) {
    QDir parent = dir;
    if (parent.cdUp()) {
      const QString ex_toml =
          parent.filePath(QStringLiteral("exercises/") + stem + QStringLiteral(".toml"));
      if (QFileInfo::exists(ex_toml)) {
        return QFileInfo(ex_toml).absoluteFilePath();
      }
    }
  }

  // 3. example.toml in the same directory or parent directory
  const QString local_example = dir.filePath(QStringLiteral("example.toml"));
  if (QFileInfo::exists(local_example)) {
    return QFileInfo(local_example).absoluteFilePath();
  }
  QDir parent = dir;
  if (parent.cdUp()) {
    const QString parent_example = parent.filePath(QStringLiteral("example.toml"));
    if (QFileInfo::exists(parent_example)) {
      return QFileInfo(parent_example).absoluteFilePath();
    }
  }
  return QString();
}

// ---------------------------------------------------------------------------
// ExamplesBrowserDialog
// ---------------------------------------------------------------------------

ExamplesBrowserDialog::ExamplesBrowserDialog(QWidget* parent, const QString& examples_dir)
    : QDialog(parent), catalog_(examples_dir) {
  setWindowTitle(QStringLiteral("EASy68K Examples & Curriculum Browser"));
  resize(980, 660);
  BuildUi();
  PopulateTree();
}

void ExamplesBrowserDialog::BuildUi() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(8, 8, 8, 8);
  root->setSpacing(6);

  // Filter bar
  auto* filter_bar = new QHBoxLayout();
  filter_bar->addWidget(new QLabel(QStringLiteral("&Track:"), this));
  track_combo_ = new QComboBox(this);
  track_combo_->addItem(QStringLiteral("All Tracks"), QString());
  track_combo_->addItem(QStringLiteral("Core Path (core)"), QStringLiteral("core"));
  track_combo_->addItem(QStringLiteral("Track S: Systems & Devices (systems)"),
                        QStringLiteral("systems"));
  track_combo_->addItem(QStringLiteral("Track C: C & C++ (c)"), QStringLiteral("c"));
  track_combo_->addItem(QStringLiteral("Track O: Optimisation (optimization)"),
                        QStringLiteral("optimization"));
  track_combo_->addItem(QStringLiteral("Capstones (projects)"), QStringLiteral("projects"));
  filter_bar->addWidget(track_combo_);

  filter_bar->addWidget(new QLabel(QStringLiteral("&Level:"), this));
  level_combo_ = new QComboBox(this);
  level_combo_->addItem(QStringLiteral("All Levels"), -1);
  for (int lvl = 0; lvl <= 5; ++lvl) {
    level_combo_->addItem(QStringLiteral("Level %1").arg(lvl), lvl);
  }
  filter_bar->addWidget(level_combo_);

  filter_bar->addWidget(new QLabel(QStringLiteral("&Filter:"), this));
  search_edit_ = new QLineEdit(this);
  search_edit_->setPlaceholderText(
      QStringLiteral("Search by ID, title, instruction, or concept..."));
  filter_bar->addWidget(search_edit_, 1);
  root->addLayout(filter_bar);

  // Main splitter: left tree, right README + metadata
  auto* splitter = new QSplitter(Qt::Horizontal, this);

  tree_ = new QTreeWidget(splitter);
  tree_->setColumnCount(5);
  tree_->setHeaderLabels({QStringLiteral("ID"), QStringLiteral("Title"), QStringLiteral("Track"),
                          QStringLiteral("Lvl"), QStringLiteral("Diff")});
  tree_->setRootIsDecorated(false);
  tree_->setAlternatingRowColors(true);
  tree_->setUniformRowHeights(true);
  tree_->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  tree_->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  tree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  tree_->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  tree_->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

  auto* right_pane = new QWidget(splitter);
  auto* right_layout = new QVBoxLayout(right_pane);
  right_layout->setContentsMargins(0, 0, 0, 0);
  right_layout->setSpacing(4);

  meta_label_ = new QLabel(right_pane);
  meta_label_->setWordWrap(true);
  meta_label_->setStyleSheet(
      QStringLiteral("QLabel { background: #f3f6fa; border: 1px solid #cfd8e3; padding: 6px; }"));
  right_layout->addWidget(meta_label_);

  readme_browser_ = new QTextBrowser(right_pane);
  readme_browser_->setOpenExternalLinks(false);
  readme_browser_->setOpenLinks(false);
  right_layout->addWidget(readme_browser_, 1);

  splitter->addWidget(tree_);
  splitter->addWidget(right_pane);
  splitter->setStretchFactor(0, 2);
  splitter->setStretchFactor(1, 3);
  splitter->setSizes({420, 540});
  root->addWidget(splitter, 1);

  // Bottom buttons
  auto* btn_bar = new QHBoxLayout();
  open_example_btn_ = new QPushButton(QStringLiteral("&Open Example in Editor"), this);
  open_exercise_btn_ = new QPushButton(QStringLiteral("Open &Exercise Starter"), this);
  open_naive_btn_ = new QPushButton(QStringLiteral("Open &Naive Baseline"), this);
  close_btn_ = new QPushButton(QStringLiteral("&Close"), this);
  open_example_btn_->setDefault(true);

  btn_bar->addWidget(open_example_btn_);
  btn_bar->addWidget(open_exercise_btn_);
  btn_bar->addWidget(open_naive_btn_);
  btn_bar->addStretch(1);
  btn_bar->addWidget(close_btn_);
  root->addLayout(btn_bar);

  connect(track_combo_, &QComboBox::currentIndexChanged, this, [this](int) { PopulateTree(); });
  connect(level_combo_, &QComboBox::currentIndexChanged, this, [this](int) { PopulateTree(); });
  connect(search_edit_, &QLineEdit::textChanged, this, [this](const QString&) { PopulateTree(); });
  connect(tree_, &QTreeWidget::currentItemChanged, this,
          [this](QTreeWidgetItem*, QTreeWidgetItem*) { OnSelectionChanged(); });
  connect(tree_, &QTreeWidget::itemDoubleClicked, this,
          [this](QTreeWidgetItem*, int) { TriggerOpenSelectedExample(); });
  connect(open_example_btn_, &QPushButton::clicked, this,
          &ExamplesBrowserDialog::TriggerOpenSelectedExample);
  connect(open_exercise_btn_, &QPushButton::clicked, this,
          &ExamplesBrowserDialog::TriggerOpenSelectedExercise);
  connect(open_naive_btn_, &QPushButton::clicked, this,
          &ExamplesBrowserDialog::TriggerOpenSelectedNaive);
  connect(close_btn_, &QPushButton::clicked, this, &QDialog::close);

  connect(readme_browser_, &QTextBrowser::anchorClicked, this, [this](const QUrl& url) {
    if (url.scheme() == QStringLiteral("example")) {
      SelectExampleById(url.host().isEmpty() ? url.path() : url.host());
    }
  });
}

void ExamplesBrowserDialog::PopulateTree() {
  const QString prev_id = SelectedEntry() != nullptr ? SelectedEntry()->id : QString();
  const QString track_filter = track_combo_->currentData().toString();
  const int level_filter = level_combo_->currentData().toInt();
  const QString search = search_edit_->text().trimmed();

  tree_->blockSignals(true);
  tree_->clear();

  QTreeWidgetItem* to_select = nullptr;
  const auto& entries = catalog_.entries();
  for (int i = 0; i < entries.size(); ++i) {
    const ExampleCatalogEntry& e = entries[i];
    if (!track_filter.isEmpty() && e.track.compare(track_filter, Qt::CaseInsensitive) != 0)
      continue;
    if (level_filter >= 0 && e.level != level_filter)
      continue;
    if (!search.isEmpty()) {
      const bool matches = e.id.contains(search, Qt::CaseInsensitive) ||
                           e.title.contains(search, Qt::CaseInsensitive) ||
                           e.readme_markdown.contains(search, Qt::CaseInsensitive) ||
                           e.mnemonics_used.contains(search.toUpper());
      if (!matches)
        continue;
    }

    auto* item = new QTreeWidgetItem(tree_);
    item->setText(0, e.id);
    item->setText(1, e.title);
    item->setText(2, e.track);
    item->setText(3, QString::number(e.level));
    item->setText(4, DifficultyBullets(e.difficulty));
    item->setData(0, kEntryIndexRole, i);

    if (to_select == nullptr || (!prev_id.isEmpty() && e.id == prev_id)) {
      to_select = item;
    }
  }

  tree_->blockSignals(false);
  if (to_select != nullptr) {
    tree_->setCurrentItem(to_select);
  }
  OnSelectionChanged();
}

const ExampleCatalogEntry* ExamplesBrowserDialog::SelectedEntry() const {
  QTreeWidgetItem* item = tree_ != nullptr ? tree_->currentItem() : nullptr;
  if (item == nullptr)
    return nullptr;
  const int idx = item->data(0, kEntryIndexRole).toInt();
  if (idx < 0 || idx >= catalog_.entries().size())
    return nullptr;
  return &catalog_.entries()[idx];
}

bool ExamplesBrowserDialog::SelectExampleById(const QString& id) {
  const QString target = id.trimmed().toUpper();
  // Clear filters if needed so the target is visible.
  for (int pass = 0; pass < 2; ++pass) {
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
      QTreeWidgetItem* item = tree_->topLevelItem(i);
      if (item->text(0).compare(target, Qt::CaseInsensitive) == 0) {
        tree_->setCurrentItem(item);
        tree_->scrollToItem(item);
        return true;
      }
    }
    if (pass == 0) {
      track_combo_->setCurrentIndex(0);
      level_combo_->setCurrentIndex(0);
      search_edit_->clear();
      PopulateTree();
    }
  }
  return false;
}

void ExamplesBrowserDialog::SetTrackFilter(const QString& track) {
  const int idx = track_combo_->findData(track);
  if (idx >= 0) {
    track_combo_->setCurrentIndex(idx);
  }
}

void ExamplesBrowserDialog::SetLevelFilter(int level) {
  const int idx = level_combo_->findData(level);
  if (idx >= 0) {
    level_combo_->setCurrentIndex(idx);
  }
}

void ExamplesBrowserDialog::SetSearchFilter(const QString& text) {
  search_edit_->setText(text);
}

void ExamplesBrowserDialog::OnSelectionChanged() {
  const ExampleCatalogEntry* entry = SelectedEntry();
  open_example_btn_->setEnabled(entry != nullptr && !entry->source_path.isEmpty());
  open_exercise_btn_->setEnabled(entry != nullptr && !entry->exercise_source_path.isEmpty());
  open_naive_btn_->setEnabled(entry != nullptr && !entry->naive_source_path.isEmpty());
  UpdateReadmePreview(entry);
}

void ExamplesBrowserDialog::UpdateReadmePreview(const ExampleCatalogEntry* entry) {
  if (entry == nullptr) {
    meta_label_->setText(QStringLiteral("No example selected."));
    readme_browser_->clear();
    return;
  }

  QString prereq_html = QStringLiteral("None");
  if (!entry->requires_ids.isEmpty()) {
    QStringList links;
    for (const QString& req : entry->requires_ids) {
      links.push_back(QStringLiteral("<a href=\"example://%1\">%1</a>").arg(req.toHtmlEscaped()));
    }
    prereq_html = links.join(QStringLiteral(", "));
  }

  QString budget_str;
  if (entry->spec.expect_max_cycles.has_value()) {
    budget_str = QStringLiteral(" | <b>Cycle Budget:</b> &le; %1 cycles")
                     .arg(*entry->spec.expect_max_cycles);
  }

  meta_label_->setText(
      QStringLiteral("<b>%1 — %2</b><br>"
                     "<b>Track:</b> %3 | <b>Level:</b> %4 | <b>Difficulty:</b> %5 | "
                     "<b>Prerequisites:</b> %6%7")
          .arg(entry->id.toHtmlEscaped(), entry->title.toHtmlEscaped(),
               entry->track.toHtmlEscaped())
          .arg(entry->level)
          .arg(DifficultyBullets(entry->difficulty))
          .arg(prereq_html, budget_str));

  readme_browser_->setMarkdown(entry->readme_markdown);
}

void ExamplesBrowserDialog::TriggerOpenSelectedExample() {
  const ExampleCatalogEntry* entry = SelectedEntry();
  if (entry == nullptr || entry->source_path.isEmpty())
    return;
  if (open_file_cb_) {
    open_file_cb_(entry->source_path, entry->readme_path);
  }
}

void ExamplesBrowserDialog::TriggerOpenSelectedExercise() {
  const ExampleCatalogEntry* entry = SelectedEntry();
  if (entry == nullptr || entry->exercise_source_path.isEmpty())
    return;
  if (open_file_cb_) {
    open_file_cb_(entry->exercise_source_path, entry->readme_path);
  }
}

void ExamplesBrowserDialog::TriggerOpenSelectedNaive() {
  const ExampleCatalogEntry* entry = SelectedEntry();
  if (entry == nullptr || entry->naive_source_path.isEmpty())
    return;
  if (open_file_cb_) {
    open_file_cb_(entry->naive_source_path, entry->readme_path);
  }
}

// ---------------------------------------------------------------------------
// Solution Checker
// ---------------------------------------------------------------------------

SolutionCheckReport CheckSimulatorStateAgainstSpec(const tools::ExampleSpec& spec,
                                                   const sim::CpuState& state, bool halted,
                                                   const sim::Memory& memory,
                                                   const QString& console_output,
                                                   const QString& spec_path,
                                                   const QString& what_to_watch) {
  SolutionCheckReport report;
  report.valid_spec = true;
  report.spec_id = QString::fromStdString(spec.id);
  report.spec_title = QString::fromStdString(spec.title);
  report.spec_path = spec_path;
  report.what_to_watch = what_to_watch;
  report.cycles = state.cycles;

  if (spec.expect_halted.has_value()) {
    if (halted == *spec.expect_halted) {
      report.passed_checks.push_back(
          QStringLiteral("halted = %1")
              .arg(halted ? QStringLiteral("true") : QStringLiteral("false")));
    } else {
      report.failures.push_back(
          QStringLiteral("expected halted = %1, got %2 (run the program to completion first)")
              .arg(*spec.expect_halted ? QStringLiteral("true") : QStringLiteral("false"),
                   halted ? QStringLiteral("true") : QStringLiteral("false")));
    }
  }

  for (const auto& [reg_name, expected_val] : spec.expect_registers) {
    const QString qreg = QString::fromStdString(reg_name);
    uint32_t actual_val = 0;
    if (!LookupRegisterValue(state, qreg, &actual_val)) {
      report.failures.push_back(QStringLiteral("unknown register %1").arg(qreg));
      continue;
    }
    if (actual_val == expected_val) {
      report.passed_checks.push_back(
          QStringLiteral("register %1 = 0x%2 (%3)")
              .arg(qreg,
                   QString::number(actual_val, 16).toUpper().rightJustified(8, QLatin1Char('0')))
              .arg(actual_val));
    } else {
      report.failures.push_back(
          QStringLiteral("register %1: expected 0x%2 (%3), got 0x%4 (%5)")
              .arg(qreg,
                   QString::number(expected_val, 16).toUpper().rightJustified(8, QLatin1Char('0')))
              .arg(expected_val)
              .arg(QString::number(actual_val, 16).toUpper().rightJustified(8, QLatin1Char('0')))
              .arg(actual_val));
    }
  }

  // Normalize line endings for console output comparison if needed.
  auto normalize_crlf = [](QString s) {
    s.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    return s;
  };
  if (spec.expect_output.has_value()) {
    const QString expected_out = QString::fromStdString(*spec.expect_output);
    if (console_output == expected_out ||
        normalize_crlf(console_output) == normalize_crlf(expected_out)) {
      report.passed_checks.push_back(QStringLiteral("console output matches expected string"));
    } else {
      report.failures.push_back(
          QStringLiteral("output mismatch: expected %1, got %2").arg(expected_out, console_output));
    }
  }

  for (const std::string& sub : spec.expect_output_contains) {
    const QString qsub = QString::fromStdString(sub);
    if (console_output.contains(qsub) ||
        normalize_crlf(console_output).contains(normalize_crlf(qsub))) {
      report.passed_checks.push_back(QStringLiteral("output contains \"%1\"").arg(qsub));
    } else {
      report.failures.push_back(
          QStringLiteral("output missing expected substring \"%1\"").arg(qsub));
    }
  }

  if (spec.expect_max_cycles.has_value()) {
    if (state.cycles <= *spec.expect_max_cycles) {
      report.passed_checks.push_back(QStringLiteral("cycles = %1 (budget <= %2)")
                                         .arg(state.cycles)
                                         .arg(*spec.expect_max_cycles));
    } else {
      report.failures.push_back(QStringLiteral("cycles %1 exceeded max_cycles %2")
                                    .arg(state.cycles)
                                    .arg(*spec.expect_max_cycles));
    }
  }

  if (spec.expect_min_cycles.has_value()) {
    if (state.cycles >= *spec.expect_min_cycles) {
      report.passed_checks.push_back(
          QStringLiteral("cycles = %1 (>= %2)").arg(state.cycles).arg(*spec.expect_min_cycles));
    } else {
      report.failures.push_back(QStringLiteral("cycles %1 below min_cycles %2")
                                    .arg(state.cycles)
                                    .arg(*spec.expect_min_cycles));
    }
  }

  for (const tools::MemoryExpectation& mem_exp : spec.expect_memory) {
    bool mem_ok = true;
    QStringList expected_hex;
    QStringList actual_hex;
    for (size_t i = 0; i < mem_exp.bytes.size(); ++i) {
      const uint32_t masked_addr = static_cast<uint32_t>(mem_exp.address + i) & sim::kAddressMask;
      const uint8_t actual_byte =
          masked_addr < memory.Size() ? memory.RawData()[masked_addr] : 0xFF;
      const uint8_t exp_byte = mem_exp.bytes[i];
      expected_hex.push_back(
          QString::number(exp_byte, 16).toUpper().rightJustified(2, QLatin1Char('0')));
      actual_hex.push_back(
          QString::number(actual_byte, 16).toUpper().rightJustified(2, QLatin1Char('0')));
      if (actual_byte != exp_byte) {
        mem_ok = false;
      }
    }
    const QString addr_str =
        QString::number(mem_exp.address, 16).toUpper().rightJustified(6, QLatin1Char('0'));
    if (mem_ok) {
      report.passed_checks.push_back(
          QStringLiteral("memory at $%1 = [%2]").arg(addr_str, actual_hex.join(QLatin1Char(' '))));
    } else {
      report.failures.push_back(QStringLiteral("memory at $%1: expected [%2], got [%3]")
                                    .arg(addr_str, expected_hex.join(QLatin1Char(' ')),
                                         actual_hex.join(QLatin1Char(' '))));
    }
  }

  report.passed = report.failures.isEmpty();
  report.summary = report.passed
                       ? QStringLiteral("PASS — %1 (%2): all %3 check(s) succeeded (%4 cycles).")
                             .arg(report.spec_id, report.spec_title)
                             .arg(report.passed_checks.size())
                             .arg(report.cycles)
                       : QStringLiteral("FAIL — %1 (%2): %3 check(s) failed.")
                             .arg(report.spec_id, report.spec_title)
                             .arg(report.failures.size());
  return report;
}

SolutionCheckReport RunHeadlessSolutionCheck(const QString& spec_path,
                                             const QString& source_override,
                                             const QString& what_to_watch) {
  SolutionCheckReport report;
  report.spec_path = spec_path;
  report.what_to_watch = what_to_watch;

  tools::ExampleSpec spec;
  std::string err;
  if (!tools::LoadExampleSpecFile(spec_path.toStdString(), &spec, &err)) {
    report.valid_spec = false;
    report.passed = false;
    report.failures.push_back(QString::fromStdString(err));
    report.summary = QStringLiteral("Could not load spec: %1").arg(QString::fromStdString(err));
    return report;
  }

  report.valid_spec = true;
  report.spec_id = QString::fromStdString(spec.id);
  report.spec_title = QString::fromStdString(spec.title);

  const tools::ExampleRunOutcome outcome =
      tools::RunExampleFile(spec_path.toStdString(), source_override.toStdString());
  report.cycles = outcome.cycles;
  report.passed = outcome.passed;

  if (outcome.passed) {
    report.passed_checks.push_back(
        QStringLiteral("Assembled and executed cleanly (%1 instructions, %2 cycles)")
            .arg(outcome.instructions_executed)
            .arg(outcome.cycles));
  } else {
    for (const std::string& f : outcome.failures) {
      report.failures.push_back(QString::fromStdString(f));
    }
  }

  report.summary = report.passed ? QStringLiteral("PASS — %1 (%2) verified cleanly (%3 cycles).")
                                       .arg(report.spec_id, report.spec_title)
                                       .arg(report.cycles)
                                 : QStringLiteral("FAIL — %1 (%2): %3 failure(s).")
                                       .arg(report.spec_id, report.spec_title)
                                       .arg(report.failures.size());
  return report;
}

SolutionCheckerDialog::SolutionCheckerDialog(QWidget* parent, const QString& initial_spec_path,
                                             const QString& what_to_watch,
                                             LiveCheckCallback live_check_cb)
    : QDialog(parent), what_to_watch_(what_to_watch), live_check_cb_(std::move(live_check_cb)) {
  setWindowTitle(QStringLiteral("Sim68K Solution Checker"));
  resize(720, 520);
  BuildUi();
  SetSpecPath(initial_spec_path, what_to_watch);
}

void SolutionCheckerDialog::BuildUi() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(8, 8, 8, 8);
  root->setSpacing(6);

  auto* path_row = new QHBoxLayout();
  path_row->addWidget(new QLabel(QStringLiteral("Spec (.toml):"), this));
  spec_edit_ = new QLineEdit(this);
  browse_btn_ = new QPushButton(QStringLiteral("&Browse..."), this);
  path_row->addWidget(spec_edit_, 1);
  path_row->addWidget(browse_btn_);
  root->addLayout(path_row);

  status_label_ = new QLabel(QStringLiteral("Select a spec and click Check Solution."), this);
  status_label_->setWordWrap(true);
  status_label_->setStyleSheet(
      QStringLiteral("QLabel { font-weight: bold; padding: 6px; border: 1px solid #cbd5e1; "
                     "background: #f8fafc; }"));
  root->addWidget(status_label_);

  details_browser_ = new QTextBrowser(this);
  root->addWidget(details_browser_, 1);

  auto* btn_row = new QHBoxLayout();
  check_live_btn_ = new QPushButton(QStringLiteral("&Check Current Simulator State"), this);
  run_clean_btn_ = new QPushButton(QStringLiteral("&Run Clean Verification"), this);
  close_btn_ = new QPushButton(QStringLiteral("&Close"), this);
  check_live_btn_->setDefault(true);

  btn_row->addWidget(check_live_btn_);
  btn_row->addWidget(run_clean_btn_);
  btn_row->addStretch(1);
  btn_row->addWidget(close_btn_);
  root->addLayout(btn_row);

  connect(browse_btn_, &QPushButton::clicked, this, [this]() {
    const QString chosen = QFileDialog::getOpenFileName(
        this, QStringLiteral("Select Example or Exercise Spec"), spec_edit_->text(),
        QStringLiteral("Example Spec (*.toml);;All files (*.*)"));
    if (!chosen.isEmpty()) {
      SetSpecPath(chosen);
    }
  });
  connect(spec_edit_, &QLineEdit::editingFinished, this, [this]() { RenderSpecOverview(); });
  connect(check_live_btn_, &QPushButton::clicked, this, [this]() { RunLiveCheck(); });
  connect(run_clean_btn_, &QPushButton::clicked, this, [this]() { RunCleanVerification(); });
  connect(close_btn_, &QPushButton::clicked, this, &QDialog::close);
}

void SolutionCheckerDialog::SetSpecPath(const QString& spec_path, const QString& what_to_watch) {
  spec_edit_->setText(spec_path);
  if (!what_to_watch.isEmpty()) {
    what_to_watch_ = what_to_watch;
  } else if (!spec_path.isEmpty()) {
    QDir dir = QFileInfo(spec_path).dir();
    QString readme = dir.filePath(QStringLiteral("README.md"));
    if (!QFileInfo::exists(readme)) {
      QDir parent = dir;
      if (parent.cdUp()) {
        readme = parent.filePath(QStringLiteral("README.md"));
      }
    }
    what_to_watch_ = ExamplesCatalog::ExtractWhatToWatch(ReadUtf8File(readme));
  }
  RenderSpecOverview();
}

QString SolutionCheckerDialog::spec_path() const {
  return spec_edit_->text().trimmed();
}

void SolutionCheckerDialog::RenderSpecOverview() {
  const QString path = spec_path();
  if (path.isEmpty() || !QFileInfo::exists(path)) {
    status_label_->setText(QStringLiteral("No example.toml spec loaded."));
    details_browser_->setHtml(QStringLiteral(
        "<p>Open an example <code>.S68</code> program or browse to an <code>example.toml</code> / "
        "<code>exercises/ex1.toml</code> file to check your solution.</p>"));
    return;
  }

  tools::ExampleSpec spec;
  std::string err;
  if (!tools::LoadExampleSpecFile(path.toStdString(), &spec, &err)) {
    status_label_->setText(QStringLiteral("Invalid spec: %1").arg(QString::fromStdString(err)));
    return;
  }

  status_label_->setText(
      QStringLiteral("Loaded %1 — %2")
          .arg(QString::fromStdString(spec.id), QString::fromStdString(spec.title)));

  QString html = QStringLiteral("<h3>%1 — %2</h3>")
                     .arg(QString::fromStdString(spec.id).toHtmlEscaped(),
                          QString::fromStdString(spec.title).toHtmlEscaped());
  if (!what_to_watch_.isEmpty()) {
    html += QStringLiteral("<p><b>What to watch:</b> %1</p>").arg(what_to_watch_.toHtmlEscaped());
  }
  html += QStringLiteral("<h4>Expected Spec Criteria:</h4><ul>");
  if (spec.expect_halted.has_value()) {
    html += QStringLiteral("<li><b>Halted:</b> %1</li>")
                .arg(*spec.expect_halted ? QStringLiteral("true") : QStringLiteral("false"));
  }
  for (const auto& [reg, val] : spec.expect_registers) {
    html +=
        QStringLiteral("<li><b>Register %1:</b> 0x%2 (%3)</li>")
            .arg(QString::fromStdString(reg).toHtmlEscaped(), QString::number(val, 16).toUpper())
            .arg(val);
  }
  if (spec.expect_output.has_value()) {
    html += QStringLiteral("<li><b>Output:</b> <code>%1</code></li>")
                .arg(QString::fromStdString(*spec.expect_output).toHtmlEscaped());
  }
  if (spec.expect_max_cycles.has_value()) {
    html += QStringLiteral("<li><b>Max Cycles:</b> &le; %1</li>").arg(*spec.expect_max_cycles);
  }
  html += QStringLiteral("</ul>");
  details_browser_->setHtml(html);
}

SolutionCheckReport SolutionCheckerDialog::RunLiveCheck() {
  const QString path = spec_path();
  if (!live_check_cb_) {
    return RunCleanVerification();
  }
  last_report_ = live_check_cb_(path);
  RenderReport(last_report_, QStringLiteral("Live Simulator State Check"));
  return last_report_;
}

SolutionCheckReport SolutionCheckerDialog::RunCleanVerification() {
  const QString path = spec_path();
  last_report_ = RunHeadlessSolutionCheck(path, QString(), what_to_watch_);
  RenderReport(last_report_, QStringLiteral("Clean Verification Run"));
  return last_report_;
}

void SolutionCheckerDialog::RenderReport(const SolutionCheckReport& report,
                                         const QString& mode_label) {
  status_label_->setText(report.summary);
  status_label_->setStyleSheet(
      report.passed
          ? QStringLiteral("QLabel { font-weight: bold; padding: 6px; border: 1px solid #16a34a; "
                           "background: #dcfce7; color: #14532d; }")
          : QStringLiteral("QLabel { font-weight: bold; padding: 6px; border: 1px solid #dc2626; "
                           "background: #fee2e2; color: #7f1d1d; }"));

  QString html = QStringLiteral("<h3>%1: %2</h3>")
                     .arg(mode_label.toHtmlEscaped(),
                          report.passed ? QStringLiteral("PASSED") : QStringLiteral("FAILED"));
  if (!report.what_to_watch.isEmpty()) {
    html +=
        QStringLiteral("<p><b>What to watch:</b> %1</p>").arg(report.what_to_watch.toHtmlEscaped());
  }
  if (!report.failures.isEmpty()) {
    html += QStringLiteral("<h4>Mismatches / Failures:</h4><ul>");
    for (const QString& f : report.failures) {
      html += QStringLiteral("<li style=\"color:#b91c1c;\">%1</li>").arg(f.toHtmlEscaped());
    }
    html += QStringLiteral("</ul>");
  }
  if (!report.passed_checks.isEmpty()) {
    html += QStringLiteral("<h4>Passed Checks:</h4><ul>");
    for (const QString& p : report.passed_checks) {
      html += QStringLiteral("<li style=\"color:#15803d;\">%1</li>").arg(p.toHtmlEscaped());
    }
    html += QStringLiteral("</ul>");
  }
  details_browser_->setHtml(html);
}

}  // namespace easym68k::gui
