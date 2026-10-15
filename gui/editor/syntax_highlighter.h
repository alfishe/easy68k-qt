// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EASY68K_GUI_EDITOR_SYNTAX_HIGHLIGHTER_H_
#define EASY68K_GUI_EDITOR_SYNTAX_HIGHLIGHTER_H_

#include <QColor>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

class QTextDocument;

namespace easym68k::editor {

// Matches FontStyle in Edit68K/asm.h.
struct TextStyle {
  QColor color = Qt::black;
  bool bold = false;
  bool italic = false;
  bool underline = false;

  bool operator==(const TextStyle& other) const {
    return color == other.color && bold == other.bold && italic == other.italic &&
           underline == other.underline;
  }
  bool operator!=(const TextStyle& other) const { return !(*this == other); }
};

// Matches the 8 syntax elements + background + preset index in
// Edit68K/editorOptions.cpp and settings.dat.
struct SyntaxTheme {
  enum PresetIndex {
    kColor1 = 0,
    kColor2 = 1,
    kMono = 2,
    kCustom = 3,
    kDisabled = 4,
  };

  TextStyle code{QColor(0, 0, 0), false, false, false};
  TextStyle unknown{QColor(128, 128, 0), false, false, false};  // clOlive
  TextStyle directive{QColor(0, 128, 0), false, false, false};  // clGreen
  TextStyle comment{QColor(0, 0, 255), false, false, false};    // clBlue
  TextStyle label{QColor(128, 0, 128), false, false, false};    // clPurple
  TextStyle structure{QColor(128, 0, 0), false, false, false};  // clMaroon
  TextStyle error{QColor(255, 0, 0), false, false, false};      // clRed
  TextStyle text{QColor(0, 128, 128), false, false, false};     // clTeal
  QColor background{255, 255, 255};                             // clWhite
  bool disabled = false;
  int preset = kColor1;

  static SyntaxTheme FromPreset(int preset_index, const QColor& bg = QColor(255, 255, 255));
};

class M68kSyntaxHighlighter : public QSyntaxHighlighter {
  Q_OBJECT

 public:
  enum class SpanKind {
    kCode,
    kComment,
    kDirective,
    kLabel,
    kUnknown,
    kStructure,
    kError,
    kText,
  };

  struct Span {
    int start = 0;
    int length = 0;
    SpanKind kind = SpanKind::kCode;
  };

  explicit M68kSyntaxHighlighter(QTextDocument* parent = nullptr);

  void SetTheme(const SyntaxTheme& theme);
  const SyntaxTheme& theme() const { return theme_; }

  void SetEnabled(bool enabled);
  bool is_enabled() const { return enabled_; }

  static QVector<Span> ClassifyLine(const QString& line);
  static QTextCharFormat FormatForStyle(const TextStyle& style);

 protected:
  void highlightBlock(const QString& text) override;

 private:
  SyntaxTheme theme_;
  bool enabled_ = true;
};

}  // namespace easym68k::editor

#endif  // EASY68K_GUI_EDITOR_SYNTAX_HIGHLIGHTER_H_
