#include "help/help_viewer.h"

#include <algorithm>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <utility>

namespace easym68k::gui {
namespace {

constexpr int kTargetRole = Qt::UserRole;

class HelpTextBrowser final : public QTextBrowser {
 public:
  explicit HelpTextBrowser(const HelpViewer* viewer, QWidget* parent = nullptr)
      : QTextBrowser(parent), viewer_(viewer) {
    setOpenLinks(false);
    setOpenExternalLinks(false);
  }

 protected:
  QVariant loadResource(int type, const QUrl& name) override {
    if (viewer_ != nullptr && !viewer_->help_dir().isEmpty()) {
      QString base_dir;
      const QString cur = viewer_->current_page().section('#', 0, 0);
      if (!cur.isEmpty()) {
        base_dir = QFileInfo(cur).path();
        if (base_dir == ".")
          base_dir.clear();
      }
      const QString rel =
          viewer_->ResolveRelativeTarget(name.toString(), base_dir).section('#', 0, 0);
      if (!rel.isEmpty()) {
        const QString abs_path = QDir(viewer_->help_dir()).filePath(rel);
        QFile file(abs_path);
        if (file.open(QIODevice::ReadOnly)) {
          return file.readAll();
        }
      }
    }
    return QTextBrowser::loadResource(type, name);
  }

 private:
  const HelpViewer* viewer_ = nullptr;
};

QString NormalizeWhitespaceUpper(const QString& text) {
  QString simplified = text.trimmed();
  simplified.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
  return simplified.toUpper();
}

QString DecodeHtmlEntities(QString text) {
  text.replace(QStringLiteral("&aring;"), QString::fromUtf8("\xC3\xA5"), Qt::CaseInsensitive);
  text.replace(QStringLiteral("&reg;"), QString::fromUtf8("\xC2\xAE"), Qt::CaseInsensitive);
  text.replace(QStringLiteral("&lt;"), QStringLiteral("<"), Qt::CaseInsensitive);
  text.replace(QStringLiteral("&gt;"), QStringLiteral(">"), Qt::CaseInsensitive);
  text.replace(QStringLiteral("&quot;"), QStringLiteral("\""), Qt::CaseInsensitive);
  text.replace(QStringLiteral("&amp;"), QStringLiteral("&"), Qt::CaseInsensitive);
  return text;
}

}  // namespace

HelpViewer::HelpViewer(QWidget* parent, const QString& help_dir)
    : QDialog(parent),
      help_dir_(help_dir.isEmpty() ? DefaultHelpDir() : help_dir),
      pdf_opener_([](const QString& pdf_path) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(pdf_path));
      }) {
  setWindowTitle(QStringLiteral("EASy68K Help"));
  resize(960, 680);

  BuildUi();
  ScanHelpDirectory();
  LoadAliasTable();
  LoadContentsTree();
  LoadIndexEntries();
  PopulateIndexList(QString());

  NavigateToPage(QStringLiteral("main.htm"));
}

QString HelpViewer::DefaultHelpDir() {
  const QString env_dir = qEnvironmentVariable("EASY68K_HELP_DIR");
  if (!env_dir.isEmpty() && QDir(env_dir).exists()) {
    return QDir(env_dir).absolutePath();
  }

  if (QCoreApplication::instance() != nullptr) {
    const QString app_dir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(app_dir).filePath(QStringLiteral("data/help")),
        QDir(app_dir).filePath(QStringLiteral("../data/help")),
        QDir(app_dir).filePath(QStringLiteral("../../data/help")),
        QDir(app_dir).filePath(QStringLiteral("../share/easy68k/help")),
    };
    for (const QString& candidate : candidates) {
      if (QDir(candidate).exists()) {
        return QDir(candidate).absolutePath();
      }
    }
  }

#ifdef EASY68K_HELP_DIR
  const QString compiled_dir = QStringLiteral(EASY68K_HELP_DIR);
  if (QDir(compiled_dir).exists()) {
    return QDir(compiled_dir).absolutePath();
  }
#endif

  return QString();
}

void HelpViewer::BuildUi() {
  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(6, 6, 6, 6);
  root_layout->setSpacing(6);

  auto* toolbar_layout = new QHBoxLayout();
  back_btn_ = new QPushButton(QStringLiteral("&Back"), this);
  forward_btn_ = new QPushButton(QStringLiteral("&Forward"), this);
  home_btn_ = new QPushButton(QStringLiteral("&Home"), this);
  quick_ref_btn_ = new QPushButton(QStringLiteral("68000 &Quick Reference (PDF)"), this);
  toolbar_layout->addWidget(back_btn_);
  toolbar_layout->addWidget(forward_btn_);
  toolbar_layout->addWidget(home_btn_);
  toolbar_layout->addWidget(quick_ref_btn_);
  toolbar_layout->addStretch(1);
  root_layout->addLayout(toolbar_layout);

  auto* splitter = new QSplitter(Qt::Horizontal, this);

  nav_tabs_ = new QTabWidget(splitter);
  toc_tree_ = new QTreeWidget(nav_tabs_);
  toc_tree_->setHeaderHidden(true);
  toc_tree_->setUniformRowHeights(true);
  nav_tabs_->addTab(toc_tree_, QStringLiteral("&Contents"));

  auto* index_page = new QWidget(nav_tabs_);
  auto* index_layout = new QVBoxLayout(index_page);
  index_layout->setContentsMargins(4, 4, 4, 4);
  index_layout->setSpacing(4);
  index_filter_ = new QLineEdit(index_page);
  index_filter_->setPlaceholderText(QStringLiteral("Type keyword to filter..."));
  index_list_ = new QListWidget(index_page);
  index_layout->addWidget(index_filter_);
  index_layout->addWidget(index_list_, 1);
  nav_tabs_->addTab(index_page, QStringLiteral("&Index"));

  browser_ = new HelpTextBrowser(this, splitter);
  splitter->addWidget(nav_tabs_);
  splitter->addWidget(browser_);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  splitter->setSizes({280, 660});

  root_layout->addWidget(splitter, 1);

  connect(back_btn_, &QPushButton::clicked, this, &HelpViewer::OnBackClicked);
  connect(forward_btn_, &QPushButton::clicked, this, &HelpViewer::OnForwardClicked);
  connect(home_btn_, &QPushButton::clicked, this, &HelpViewer::OnHomeClicked);
  connect(quick_ref_btn_, &QPushButton::clicked, this, &HelpViewer::OpenQuickReferencePdf);

  connect(toc_tree_, &QTreeWidget::itemClicked, this, &HelpViewer::OnTocItemActivated);
  connect(toc_tree_, &QTreeWidget::itemActivated, this, &HelpViewer::OnTocItemActivated);

  connect(index_filter_, &QLineEdit::textChanged, this, &HelpViewer::OnIndexFilterChanged);
  connect(index_filter_, &QLineEdit::returnPressed, this, &HelpViewer::OnIndexFilterReturnPressed);
  connect(index_list_, &QListWidget::itemClicked, this, &HelpViewer::OnIndexItemActivated);
  connect(index_list_, &QListWidget::itemActivated, this, &HelpViewer::OnIndexItemActivated);

  connect(browser_, &QTextBrowser::anchorClicked, this, &HelpViewer::OnAnchorClicked);

  UpdateNavigationButtons();
}

void HelpViewer::ScanHelpDirectory() {
  file_lookup_.clear();
  basename_lookup_.clear();
  if (help_dir_.isEmpty() || !QDir(help_dir_).exists())
    return;

  const QDir root(help_dir_);
  QDirIterator it(help_dir_, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString abs_path = it.next();
    const QString rel_path = QDir::cleanPath(root.relativeFilePath(abs_path));
    file_lookup_.insert(rel_path.toLower(), rel_path);

    const QString base_lower = QFileInfo(rel_path).fileName().toLower();
    // Prefer root-level files when basenames collide (e.g., BasicOperation.htm
    // vs SIMHELP/BasicOperation.htm).
    if (!basename_lookup_.contains(base_lower) || !rel_path.contains('/')) {
      basename_lookup_.insert(base_lower, rel_path);
    }
  }
}

QString HelpViewer::ResolveRelativeTarget(const QString& relative_target,
                                          const QString& base_dir) const {
  QString trimmed = relative_target.trimmed();
  if (trimmed.isEmpty())
    return QString();

  // Strip file:// url prefix if present cleanly.
  if (trimmed.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
    const QUrl url(trimmed);
    trimmed = url.toLocalFile();
    if (!url.fragment().isEmpty()) {
      trimmed += QLatin1Char('#') + url.fragment();
    }
  }

  const int hash_pos = trimmed.indexOf(QLatin1Char('#'));
  QString path_part = (hash_pos >= 0) ? trimmed.left(hash_pos) : trimmed;
  const QString fragment_part = (hash_pos >= 0) ? trimmed.mid(hash_pos) : QString();

  path_part.replace(QLatin1Char('\\'), QLatin1Char('/'));
  while (path_part.startsWith(QStringLiteral("./"))) {
    path_part.remove(0, 2);
  }

  if (path_part.isEmpty()) {
    if (current_page_.isEmpty())
      return QString();
    const QString cur_base = current_page_.section(QLatin1Char('#'), 0, 0);
    return cur_base + fragment_part;
  }

  // If path_part is an absolute path inside help_dir_, make it relative.
  if (QFileInfo(path_part).isAbsolute() && !help_dir_.isEmpty()) {
    const QString rel = QDir(help_dir_).relativeFilePath(path_part);
    if (!rel.startsWith(QStringLiteral(".."))) {
      path_part = rel;
    }
  }

  QStringList candidates;
  if (!base_dir.isEmpty()) {
    candidates.push_back(QDir::cleanPath(base_dir + QLatin1Char('/') + path_part));
  }
  candidates.push_back(QDir::cleanPath(path_part));

  for (const QString& candidate : candidates) {
    const auto it = file_lookup_.constFind(candidate.toLower());
    if (it != file_lookup_.cend()) {
      return it.value() + fragment_part;
    }
  }

  const QString base_lower = QFileInfo(path_part).fileName().toLower();
  const auto base_it = basename_lookup_.constFind(base_lower);
  if (base_it != basename_lookup_.cend()) {
    return base_it.value() + fragment_part;
  }

  return QString();
}

void HelpViewer::LoadAliasTable() {
  context_map_.clear();

  const QString ali_rel = ResolveRelativeTarget(QStringLiteral("ez68k.ali")).section('#', 0, 0);
  if (!ali_rel.isEmpty()) {
    QFile ali_file(QDir(help_dir_).filePath(ali_rel));
    if (ali_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream in(&ali_file);
      while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char(';')) ||
            line.startsWith(QLatin1Char('#'))) {
          continue;
        }
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
          continue;
        const QString idh_key = line.left(eq).trimmed().toUpper();
        const QString raw_target = line.mid(eq + 1).trimmed();
        const QString resolved = ResolveRelativeTarget(raw_target);
        if (resolved.isEmpty())
          continue;

        context_map_.insert(idh_key, resolved);
        if (idh_key.startsWith(QStringLiteral("IDH_"))) {
          const QString short_key = idh_key.mid(4);
          context_map_.insert(short_key, resolved);
          if (short_key.startsWith(QStringLiteral("DIRECT_"))) {
            context_map_.insert(short_key.mid(7), resolved);
          }
        }
      }
    }
  }

  auto map_to_idh = [this](const QString& keyword, const QString& idh_name) {
    const auto it = context_map_.constFind(idh_name.toUpper());
    if (it != context_map_.cend()) {
      const QString target = it.value();
      context_map_.insert(keyword.toUpper(), target);
    }
  };

  // Explicit context mappings from Edit68K/help.cpp, Sim68K/help.cpp, and
  // EASyBIN/help.cpp (including fixes for original lookup omissions such as
  // "SIM_BASIC", "SIM_OPTIONS", "EDIT_OPTIONS", and "CHUCK").
  map_to_idh(QStringLiteral("EASY68K"), QStringLiteral("IDH_INTRO"));
  map_to_idh(QStringLiteral("MAIN"), QStringLiteral("IDH_INTRO"));
  map_to_idh(QStringLiteral("INTRO"), QStringLiteral("IDH_INTRO"));
  map_to_idh(QStringLiteral("68000"), QStringLiteral("IDH_QUICKREF"));
  map_to_idh(QStringLiteral("ASSEMBLER"), QStringLiteral("IDH_ASSEMBLER_BASIC_OPERATION"));
  map_to_idh(QStringLiteral("OPTIONS"), QStringLiteral("IDH_EDIT_OPTIONS"));
  map_to_idh(QStringLiteral("EDIT_OPTIONS"), QStringLiteral("IDH_EDIT_OPTIONS"));

  for (const char* bcc : {"BCC", "BCS", "BEQ", "BNE", "BGE", "BGT", "BHI", "BLE", "BLS", "BLT",
                          "BMI", "BPL", "BVC", "BVS", "BRA", "BHS", "BLO"}) {
    map_to_idh(QString::fromLatin1(bcc), QStringLiteral("IDH_BCC"));
  }
  for (const char* dbcc :
       {"DBCC", "DBCS", "DBEQ", "DBNE", "DBGE", "DBGT", "DBHI", "DBLE", "DBLS", "DBLT", "DBMI",
        "DBPL", "DBVC", "DBVS", "DBRA", "DBF", "DBT", "DBHS", "DBLO"}) {
    map_to_idh(QString::fromLatin1(dbcc), QStringLiteral("IDH_DBCC"));
  }
  for (const char* scc :
       {"SCC", "SCS", "SEQ", "SGE", "SGT", "SHI", "SLE", "SLS", "SLT", "SME", "SMI",
        "SNE", "SPL", "SVC", "SVS", "SSF", "SST", "SF",  "ST",  "SHS", "SLO"}) {
    map_to_idh(QString::fromLatin1(scc), QStringLiteral("IDH_SCC"));
  }
  for (const char* cond :
       {"CONDITIONAL", "ENDC", "IFC", "IFEQ", "IFGE", "IFGT", "IFLE", "IFLT", "IFNC", "IFNE"}) {
    map_to_idh(QString::fromLatin1(cond), QStringLiteral("IDH_CONDITIONAL"));
  }
  for (const char* opt :
       {"OPT", "MEX", "NOMEX", "NOSEX", "NOWAR", "SEX", "WAR", "CRE", "NOCRE", "CEX", "NOCEX"}) {
    map_to_idh(QString::fromLatin1(opt), QStringLiteral("IDH_DIRECT_OPT"));
  }

  map_to_idh(QStringLiteral("BREAKPOINT"), QStringLiteral("IDH_SIM_ADVBREAK"));
  map_to_idh(QStringLiteral("SIMULATOR"), QStringLiteral("IDH_SIM_BASIC"));
  map_to_idh(QStringLiteral("EXCEPTIONS"), QStringLiteral("IDH_SIM_EXCEPTIONS"));
  map_to_idh(QStringLiteral("HARDWARE"), QStringLiteral("IDH_SIM_HARDWARE"));
  map_to_idh(QStringLiteral("STACK"), QStringLiteral("IDH_SIM_STACK"));

  map_to_idh(QStringLiteral("DBLOOP"), QStringLiteral("IDH_STRUCCONTROL_DBLOOP"));
  map_to_idh(QStringLiteral("UNLESS"), QStringLiteral("IDH_STRUCCONTROL_DBLOOP"));
  map_to_idh(QStringLiteral("EXPRESSION"), QStringLiteral("IDH_STRUCCONTROL_EXSYNTAX"));
  map_to_idh(QStringLiteral("FOR"), QStringLiteral("IDH_STRUCCONTROL_FOR"));
  map_to_idh(QStringLiteral("ENDF"), QStringLiteral("IDH_STRUCCONTROL_FOR"));
  map_to_idh(QStringLiteral("IF"), QStringLiteral("IDH_STRUCCONTROL_IF"));
  map_to_idh(QStringLiteral("ENDI"), QStringLiteral("IDH_STRUCCONTROL_IF"));
  map_to_idh(QStringLiteral("STRUCTURED"), QStringLiteral("IDH_STRUCCONTROL_INTRO"));
  map_to_idh(QStringLiteral("REPEAT"), QStringLiteral("IDH_STRUCCONTROL_REPEAT"));
  map_to_idh(QStringLiteral("UNTIL"), QStringLiteral("IDH_STRUCCONTROL_REPEAT"));
  map_to_idh(QStringLiteral("WHILE"), QStringLiteral("IDH_STRUCCONTROL_WHILE"));
  map_to_idh(QStringLiteral("ENDW"), QStringLiteral("IDH_STRUCCONTROL_WHILE"));
  map_to_idh(QStringLiteral("ENDM"), QStringLiteral("IDH_MACRO"));

  map_to_idh(QStringLiteral("ERROR"), QStringLiteral("IDH_ASSEMBLER_ERROR"));
  map_to_idh(QStringLiteral("WARNING"), QStringLiteral("IDH_ASSEMBLER_ERROR"));
  map_to_idh(QStringLiteral("FILE"), QStringLiteral("IDH_IO_FILE"));
  map_to_idh(QStringLiteral("GRAPHICS"), QStringLiteral("IDH_IO_GRAPHICS"));
  map_to_idh(QStringLiteral("NETWORK"), QStringLiteral("IDH_IO_NETWORK"));
  map_to_idh(QStringLiteral("MOUSE"), QStringLiteral("IDH_IO_PERIPHERAL"));
  map_to_idh(QStringLiteral("PERIPHERAL"), QStringLiteral("IDH_IO_PERIPHERAL"));
  map_to_idh(QStringLiteral("SERIAL"), QStringLiteral("IDH_IO_SERIAL"));
  map_to_idh(QStringLiteral("SOUND"), QStringLiteral("IDH_IO_SOUND"));
  map_to_idh(QStringLiteral("TEXT"), QStringLiteral("IDH_IO_TEXT"));
  map_to_idh(QStringLiteral("GOD"), QStringLiteral("IDH_PSALMS"));
  map_to_idh(QStringLiteral("MOVE USP"), QStringLiteral("IDH_MOVE_USP"));

  for (const char* author : {"CREDITS", "CHUCK", "CHARLES", "CURT", "AARON"}) {
    map_to_idh(QString::fromLatin1(author), QStringLiteral("IDH_CREDITS"));
  }
}

void HelpViewer::LoadContentsTree() {
  toc_tree_->clear();
  const QString hhc_rel = ResolveRelativeTarget(QStringLiteral("contents.hhc")).section('#', 0, 0);
  if (hhc_rel.isEmpty())
    return;

  QFile hhc_file(QDir(help_dir_).filePath(hhc_rel));
  if (!hhc_file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  QTextStream in(&hhc_file);
  const QString content = in.readAll();

  const QRegularExpression token_re(
      QStringLiteral("<(/?UL|LI)\\b|<param\\s+name=\"([^\"]+)\"\\s+value=\"([^\"]*)\""),
      QRegularExpression::CaseInsensitiveOption);

  QVector<QTreeWidgetItem*> parent_stack;
  QTreeWidgetItem* current_item = nullptr;

  auto it = token_re.globalMatch(content);
  while (it.hasNext()) {
    const QRegularExpressionMatch match = it.next();
    const QString tag = match.captured(1).toUpper();
    if (tag == QStringLiteral("UL")) {
      parent_stack.push_back(current_item);
    } else if (tag == QStringLiteral("/UL")) {
      if (!parent_stack.isEmpty()) {
        current_item = parent_stack.takeLast();
      }
    } else if (tag == QStringLiteral("LI")) {
      QTreeWidgetItem* parent = parent_stack.isEmpty() ? nullptr : parent_stack.back();
      if (parent != nullptr) {
        current_item = new QTreeWidgetItem(parent);
      } else {
        current_item = new QTreeWidgetItem(toc_tree_);
      }
    } else if (current_item != nullptr) {
      const QString param_name = match.captured(2).toLower();
      const QString param_value = match.captured(3).trimmed();
      if (param_name == QStringLiteral("name")) {
        if (current_item->text(0).isEmpty()) {
          current_item->setText(0, DecodeHtmlEntities(param_value));
        }
      } else if (param_name == QStringLiteral("local")) {
        const QString resolved = ResolveRelativeTarget(param_value);
        if (!resolved.isEmpty()) {
          current_item->setData(0, kTargetRole, resolved);
        }
      }
    }
  }

  toc_tree_->expandToDepth(0);
}

void HelpViewer::LoadIndexEntries() {
  index_entries_.clear();
  const QString hhk_rel = ResolveRelativeTarget(QStringLiteral("index.hhk")).section('#', 0, 0);
  if (hhk_rel.isEmpty())
    return;

  QFile hhk_file(QDir(help_dir_).filePath(hhk_rel));
  if (!hhk_file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  QTextStream in(&hhk_file);
  const QString content = in.readAll();

  const QRegularExpression object_re(
      QStringLiteral("<OBJECT\\s+type=\"text/sitemap\"[^>]*>(.*?)</OBJECT>"),
      QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
  const QRegularExpression param_re(
      QStringLiteral("<param\\s+name=\"([^\"]+)\"\\s+value=\"([^\"]*)\""),
      QRegularExpression::CaseInsensitiveOption);

  auto obj_it = object_re.globalMatch(content);
  while (obj_it.hasNext()) {
    const QString body = obj_it.next().captured(1);
    QString first_name;
    QString local_target;
    auto param_it = param_re.globalMatch(body);
    while (param_it.hasNext()) {
      const QRegularExpressionMatch pm = param_it.next();
      const QString pname = pm.captured(1).toLower();
      const QString pval = pm.captured(2).trimmed();
      if (pname == QStringLiteral("name") && first_name.isEmpty()) {
        first_name = DecodeHtmlEntities(pval);
      } else if (pname == QStringLiteral("local") && local_target.isEmpty()) {
        local_target = ResolveRelativeTarget(pval);
      }
    }
    if (!first_name.isEmpty() && !local_target.isEmpty()) {
      index_entries_.push_back({first_name, local_target});
    }
  }

  std::sort(index_entries_.begin(), index_entries_.end(),
            [](const IndexEntry& a, const IndexEntry& b) {
              return QString::compare(a.title, b.title, Qt::CaseInsensitive) < 0;
            });
}

void HelpViewer::PopulateIndexList(const QString& filter) {
  index_list_->clear();
  const QString needle = filter.trimmed();
  for (const IndexEntry& entry : index_entries_) {
    if (!needle.isEmpty() && !entry.title.contains(needle, Qt::CaseInsensitive)) {
      continue;
    }
    auto* item = new QListWidgetItem(entry.title, index_list_);
    item->setData(kTargetRole, entry.target);
  }
  if (index_list_->count() > 0) {
    index_list_->setCurrentRow(0);
  }
}

QString HelpViewer::ResolveContextPage(const QString& context) const {
  const QString default_page = ResolveRelativeTarget(QStringLiteral("main.htm"));
  const QString normalized = NormalizeWhitespaceUpper(context);
  if (normalized.isEmpty()) {
    return default_page;
  }

  // 1. Direct lookup in context alias table (includes IDH_* and short keys).
  const auto direct_it = context_map_.constFind(normalized);
  if (direct_it != context_map_.cend()) {
    return direct_it.value();
  }

  // 2. Direct relative path lookup if caller passed a filename (e.g.
  // "easybin.htm" or "SIMHELP/memory.htm").
  if (normalized.endsWith(QStringLiteral(".HTM")) || normalized.endsWith(QStringLiteral(".HTML"))) {
    const QString resolved = ResolveRelativeTarget(context);
    if (!resolved.isEmpty())
      return resolved;
  }

  // 3. Strip size suffix (.B, .W, .L, .S) on 68000 mnemonics (e.g. "MOVE.W").
  const int dot_pos = normalized.indexOf(QLatin1Char('.'));
  if (dot_pos > 0) {
    const QString stem = normalized.left(dot_pos);
    const auto stem_it = context_map_.constFind(stem);
    if (stem_it != context_map_.cend()) {
      return stem_it.value();
    }
  }

  // 4. First alphanumeric token (e.g. when a line or multi-word selection is
  // passed from the editor).
  const QRegularExpression word_re(QStringLiteral("[A-Z0-9_]+"));
  const QRegularExpressionMatch word_match = word_re.match(normalized);
  if (word_match.hasMatch()) {
    const QString token = word_match.captured(0);
    const auto token_it = context_map_.constFind(token);
    if (token_it != context_map_.cend()) {
      return token_it.value();
    }
  }

  return default_page;
}

bool HelpViewer::ShowContext(const QString& context) {
  const QString page = ResolveContextPage(context);
  const bool ok = NavigateToPage(page);
  show();
  raise();
  activateWindow();
  return ok;
}

bool HelpViewer::NavigateToPage(const QString& relative_target) {
  QString base_dir;
  if (!current_page_.isEmpty()) {
    base_dir = QFileInfo(current_page_.section('#', 0, 0)).path();
    if (base_dir == ".")
      base_dir.clear();
  }
  const QString resolved = ResolveRelativeTarget(relative_target, base_dir);
  if (resolved.isEmpty())
    return false;
  return LoadTargetInternal(resolved, /*record_history=*/true);
}

bool HelpViewer::OpenQuickReferencePdf() {
  const QString pdf_rel =
      ResolveRelativeTarget(QStringLiteral("EASy68KQuickRef.pdf")).section('#', 0, 0);
  if (pdf_rel.isEmpty())
    return false;
  return OpenPdfTarget(pdf_rel);
}

bool HelpViewer::OpenPdfTarget(const QString& canonical_rel_path) {
  if (help_dir_.isEmpty())
    return false;
  const QString abs_path = QDir(help_dir_).filePath(canonical_rel_path);
  if (!QFileInfo::exists(abs_path))
    return false;
  if (pdf_opener_) {
    pdf_opener_(abs_path);
  }
  return true;
}

bool HelpViewer::LoadTargetInternal(const QString& canonical_target, bool record_history) {
  if (help_dir_.isEmpty())
    return false;

  const QString rel_file = canonical_target.section(QLatin1Char('#'), 0, 0);
  const QString fragment = canonical_target.contains(QLatin1Char('#'))
                               ? canonical_target.section(QLatin1Char('#'), 1)
                               : QString();

  if (rel_file.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
    return OpenPdfTarget(rel_file);
  }

  const QString abs_path = QDir(help_dir_).filePath(rel_file);
  QFile file(abs_path);
  if (!file.open(QIODevice::ReadOnly))
    return false;
  const QString html = QString::fromUtf8(file.readAll());

  current_page_ = canonical_target;
  browser_->document()->setBaseUrl(
      QUrl::fromLocalFile(QFileInfo(abs_path).absolutePath() + QLatin1Char('/')));
  browser_->setHtml(html);
  if (!fragment.isEmpty()) {
    browser_->scrollToAnchor(fragment);
  }

  const QString page_title = browser_->documentTitle().trimmed();
  if (!page_title.isEmpty()) {
    setWindowTitle(QStringLiteral("EASy68K Help - %1").arg(page_title));
  } else {
    setWindowTitle(QStringLiteral("EASy68K Help"));
  }

  if (record_history) {
    RecordHistory(canonical_target);
  }
  SyncTocSelection(rel_file);
  UpdateNavigationButtons();
  return true;
}

void HelpViewer::RecordHistory(const QString& target) {
  if (history_index_ >= 0 && history_index_ < history_.size() &&
      history_[history_index_].compare(target, Qt::CaseInsensitive) == 0) {
    return;
  }
  while (history_.size() > history_index_ + 1) {
    history_.removeLast();
  }
  history_.push_back(target);
  history_index_ = history_.size() - 1;
}

void HelpViewer::SyncTocSelection(const QString& relative_page) {
  QTreeWidgetItemIterator it(toc_tree_);
  while (*it != nullptr) {
    QTreeWidgetItem* item = *it;
    const QString item_target = item->data(0, kTargetRole).toString().section('#', 0, 0);
    if (!item_target.isEmpty() && item_target.compare(relative_page, Qt::CaseInsensitive) == 0) {
      toc_tree_->blockSignals(true);
      toc_tree_->setCurrentItem(item);
      QTreeWidgetItem* parent = item->parent();
      while (parent != nullptr) {
        parent->setExpanded(true);
        parent = parent->parent();
      }
      toc_tree_->blockSignals(false);
      return;
    }
    ++it;
  }
}

void HelpViewer::UpdateNavigationButtons() {
  if (back_btn_ != nullptr) {
    back_btn_->setEnabled(history_index_ > 0);
  }
  if (forward_btn_ != nullptr) {
    forward_btn_->setEnabled(history_index_ >= 0 && history_index_ + 1 < history_.size());
  }
}

QString HelpViewer::current_html() const {
  return browser_ != nullptr ? browser_->toHtml() : QString();
}

QString HelpViewer::current_plain_text() const {
  return browser_ != nullptr ? browser_->toPlainText() : QString();
}

void HelpViewer::OnTocItemActivated(QTreeWidgetItem* item, int /*column*/) {
  if (item == nullptr)
    return;
  const QString target = item->data(0, kTargetRole).toString();
  if (!target.isEmpty()) {
    NavigateToPage(target);
  } else {
    item->setExpanded(!item->isExpanded());
  }
}

void HelpViewer::OnIndexFilterChanged(const QString& text) {
  PopulateIndexList(text);
}

void HelpViewer::OnIndexFilterReturnPressed() {
  if (index_list_->currentItem() != nullptr) {
    OnIndexItemActivated(index_list_->currentItem());
  }
}

void HelpViewer::OnIndexItemActivated(QListWidgetItem* item) {
  if (item == nullptr)
    return;
  const QString target = item->data(kTargetRole).toString();
  if (!target.isEmpty()) {
    NavigateToPage(target);
  }
}

void HelpViewer::OnAnchorClicked(const QUrl& url) {
  const QString scheme = url.scheme().toLower();
  if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https") ||
      scheme == QStringLiteral("mailto")) {
    QDesktopServices::openUrl(url);
    return;
  }

  NavigateToPage(url.toString());
}

void HelpViewer::OnBackClicked() {
  if (history_index_ > 0) {
    --history_index_;
    LoadTargetInternal(history_[history_index_], /*record_history=*/false);
  }
}

void HelpViewer::OnForwardClicked() {
  if (history_index_ >= 0 && history_index_ + 1 < history_.size()) {
    ++history_index_;
    LoadTargetInternal(history_[history_index_], /*record_history=*/false);
  }
}

void HelpViewer::OnHomeClicked() {
  NavigateToPage(QStringLiteral("main.htm"));
}

}  // namespace easym68k::gui
