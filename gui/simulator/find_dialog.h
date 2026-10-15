// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The Sim68K Find dialog (findDialogS.dfm / findDialogS.cpp):
//  - "Find what:" text field; changing text resets search to top of listing;
//  - "Find  (F3)" button (default when text is non-empty); first click searches
//    from top, subsequent clicks or F3 search for next match;
//  - "Cancel" button closes the dialog.

#ifndef EASY68K_GUI_SIMULATOR_FIND_DIALOG_H_
#define EASY68K_GUI_SIMULATOR_FIND_DIALOG_H_

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

namespace easym68k::gui {

class FindDialog : public QDialog {
  Q_OBJECT

 public:
  explicit FindDialog(QWidget* parent = nullptr);
  ~FindDialog() override = default;

  QString Text() const { return find_text_->text(); }
  void SetText(const QString& text) { find_text_->setText(text); }

  QLineEdit* FindTextEdit() const { return find_text_; }
  QPushButton* FindNextButton() const { return find_next_btn_; }
  QPushButton* CancelButton() const { return cancel_btn_; }

 signals:
  void FindRequested(const QString& text, bool next);

 protected:
  void showEvent(QShowEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  void OnTextChanged(const QString& text);
  void OnFindNextClicked();

  QLineEdit* find_text_ = nullptr;
  QPushButton* find_next_btn_ = nullptr;
  QPushButton* cancel_btn_ = nullptr;
  bool find_next_ = false;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_FIND_DIALOG_H_
