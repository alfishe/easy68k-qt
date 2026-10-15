#ifndef EASYM68K_GUI_HELP_HELP_VIEWER_H_
#define EASYM68K_GUI_HELP_HELP_VIEWER_H_

#include <functional>
#include <QDialog>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include "help/examples_browser.h"

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTabWidget;
class QTextBrowser;
class QTreeWidget;
class QTreeWidgetItem;

namespace easym68k::gui {

// Cross-platform replacement for the Windows HtmlHelp (.chm) viewer used by
// Sim68K, Edit68K, and EASyBIN (Help/ez68k.ali, Help/contents.hhc,
// Help/index.hhk, and the 200+ HTML reference pages in data/help/).
class HelpViewer : public QDialog {
  Q_OBJECT

 public:
  struct IndexEntry {
    QString title;
    QString target;
  };

  explicit HelpViewer(QWidget* parent = nullptr, const QString& help_dir = QString());

  static QString DefaultHelpDir();

  // Resolves a context string (e.g. "SIM_BASIC", "EASyBIN", "INTRO", "MOVE",
  // "BNE", "DC", "EDIT_OPTIONS", "CHUCK") to its canonical relative path in
  // the help directory (falling back to "main.htm").
  QString ResolveContextPage(const QString& context) const;

  // Resolves a relative path (with optional #fragment) case-insensitively
  // against the help directory. Returns the canonical relative path (with
  // optional #fragment), or empty string if the file does not exist.
  QString ResolveRelativeTarget(const QString& relative_target,
                                const QString& base_dir = QString()) const;

  // Resolves `context`, loads the corresponding help page, and shows/raises
  // the viewer window.
  bool ShowContext(const QString& context);

  // Navigates directly to a relative path (e.g. "SIMHELP/basicoperation.htm"
  // or "Reference/68ks10.htm#*** CHK ***").
  bool NavigateToPage(const QString& relative_target);

  // Opens EASy68KQuickRef.pdf via the configured PDF opener callback.
  bool OpenQuickReferencePdf();

  void SetPdfOpener(std::function<void(const QString&)> opener) { pdf_opener_ = std::move(opener); }
  void SetExampleActivationCallback(std::function<void(const QString& example_id)> cb) {
    example_activation_cb_ = std::move(cb);
  }

  const ExamplesCatalog& examples_catalog() const { return examples_catalog_; }
  const QString& help_dir() const { return help_dir_; }
  const QString& current_page() const { return current_page_; }
  QString current_html() const;
  QString current_plain_text() const;

  QTabWidget* nav_tabs() const { return nav_tabs_; }
  QTreeWidget* toc_tree() const { return toc_tree_; }
  QLineEdit* index_filter() const { return index_filter_; }
  QListWidget* index_list() const { return index_list_; }
  QTextBrowser* browser() const { return browser_; }

 private:
  void BuildUi();
  void ScanHelpDirectory();
  void LoadAliasTable();
  void LoadContentsTree();
  void LoadIndexEntries();
  void PopulateIndexList(const QString& filter);
  void SyncTocSelection(const QString& relative_page);
  void UpdateNavigationButtons();
  void RecordHistory(const QString& target);
  bool LoadTargetInternal(const QString& canonical_target, bool record_history);
  bool OpenPdfTarget(const QString& canonical_rel_path);

  void OnTocItemActivated(QTreeWidgetItem* item, int column);
  void OnIndexFilterChanged(const QString& text);
  void OnIndexFilterReturnPressed();
  void OnIndexItemActivated(QListWidgetItem* item);
  void OnAnchorClicked(const QUrl& url);
  void OnBackClicked();
  void OnForwardClicked();
  void OnHomeClicked();

  QString help_dir_;
  // Lowercase relative path -> canonical relative path on disk (using '/').
  QHash<QString, QString> file_lookup_;
  // Lowercase basename -> canonical relative path on disk (if unique or root).
  QHash<QString, QString> basename_lookup_;
  // Uppercase context keyword -> canonical relative path.
  QHash<QString, QString> context_map_;
  QVector<IndexEntry> index_entries_;

  QString current_page_;
  QStringList history_;
  int history_index_ = -1;
  std::function<void(const QString&)> pdf_opener_;
  std::function<void(const QString&)> example_activation_cb_;
  ExamplesCatalog examples_catalog_;
  // Canonical relative page path -> list of instruction mnemonics documented on that page.
  QHash<QString, QStringList> page_mnemonics_;

  QTabWidget* nav_tabs_ = nullptr;
  QTreeWidget* toc_tree_ = nullptr;
  QLineEdit* index_filter_ = nullptr;
  QListWidget* index_list_ = nullptr;
  QPushButton* back_btn_ = nullptr;
  QPushButton* forward_btn_ = nullptr;
  QPushButton* home_btn_ = nullptr;
  QPushButton* quick_ref_btn_ = nullptr;
  QTextBrowser* browser_ = nullptr;
};

}  // namespace easym68k::gui

#endif  // EASYM68K_GUI_HELP_HELP_VIEWER_H_
