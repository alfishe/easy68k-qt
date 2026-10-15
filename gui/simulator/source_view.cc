// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "source_view.h"

#include <algorithm>
#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>

#include "listing.h"

namespace easym68k::gui {

namespace {

constexpr int kGutterWidth = 14;

// ListBox1DrawItem(): a tab advances to the next multiple of 8 columns.
QString ExpandTabs(const QString& line) {
  QString out;
  out.reserve(line.size() + 16);
  for (QChar c : line) {
    if (c == QLatin1Char('\t')) {
      const int spaces = 8 - out.size() % 8;
      out.append(QString(spaces, QLatin1Char(' ')));
    } else {
      out.append(c);
    }
  }
  return out;
}

}  // namespace

// breakP: the dots beside the listing rows.
class SourceView::GutterWidget : public QWidget {
 public:
  explicit GutterWidget(SourceView* view) : QWidget(view), view_(view) {
    setFixedWidth(kGutterWidth);
  }

 protected:
  void paintEvent(QPaintEvent*) override {
    QPainter p(this);
    p.fillRect(rect(), palette().window());
    QListWidget* list = view_->list_;
    if (list->count() == 0)
      return;
    const int dy =
        list->viewport()->mapTo(view_, QPoint(0, 0)).y() - mapTo(view_, QPoint(0, 0)).y();
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    const int first = std::max(0, list->indexAt(QPoint(1, 1)).row());
    for (int row = first; row < list->count(); row++) {
      const QRect r = list->visualItemRect(list->item(row));
      if (r.top() > list->viewport()->height())
        break;
      const auto address = view_->CodeLineAddress(row);
      if (!address)
        continue;
      const bool on = view_->has_breakpoint_ && view_->has_breakpoint_(*address);
      p.setBrush(on ? QColor(Qt::red) : QColor(0, 160, 0));
      const int d = std::min(r.height() - 4, kGutterWidth - 4);
      p.drawEllipse(QRect((kGutterWidth - d) / 2, r.top() + dy + (r.height() - d) / 2, d, d));
    }
  }

  // breakPMouseDown(): left click toggles the breakpoint of a code line.
  void mousePressEvent(QMouseEvent* event) override {
    if (event->button() != Qt::LeftButton)
      return;
    const int y = mapTo(view_, event->position().toPoint()).y();
    const int row = view_->RowAt(y);
    if (const auto address = view_->CodeLineAddress(row))
      emit view_->BreakpointToggled(*address);
  }

  // BreakMenu: Clear All PC Breakpoints.
  void contextMenuEvent(QContextMenuEvent* event) override {
    QMenu menu(this);
    QAction* clear = menu.addAction(QStringLiteral("Clear All PC Breakpoints"));
    if (menu.exec(event->globalPos()) == clear)
      emit view_->ClearAllBreakpoints();
  }

 private:
  SourceView* view_;
};

// HeadingsLbl: scrolls with the listing up to HEADING_LIMIT columns.
class SourceView::HeadingWidget : public QWidget {
 public:
  explicit HeadingWidget(SourceView* view) : QWidget(view), view_(view) {}
  void Update() {
    setFixedHeight(view_->list_->fontMetrics().height() + 2);
    update();
  }

 protected:
  void paintEvent(QPaintEvent*) override {
    QPainter p(this);
    QListWidget* list = view_->list_;
    p.setFont(list->font());
    const QFontMetrics fm = list->fontMetrics();
    const int limit = (kHeadingLimit - 1) * fm.horizontalAdvance(QLatin1Char('W'));
    const int scroll = std::min(list->horizontalScrollBar()->value(), limit);
    const int x = list->viewport()->mapTo(view_, QPoint(0, 0)).x() - mapTo(view_, QPoint(0, 0)).x();
    p.drawText(x + 3 - scroll, fm.ascent() + 1, QString::fromLatin1(kHeading));
  }

 private:
  SourceView* view_;
};

SourceView::SourceView(QWidget* parent) : QWidget(parent) {
  list_ = new QListWidget(this);
  list_->setUniformItemSizes(true);
  list_->setSelectionMode(QAbstractItemView::SingleSelection);
  list_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);  // ScrollBar1
  auto* gutter = new GutterWidget(this);
  gutter_ = gutter;
  auto* heading = new HeadingWidget(this);
  heading_ = heading;

  auto* rows = new QHBoxLayout;
  rows->setContentsMargins(0, 0, 0, 0);
  rows->setSpacing(0);
  rows->addWidget(gutter_);
  rows->addWidget(list_, 1);
  auto* column = new QVBoxLayout(this);
  column->setContentsMargins(0, 0, 0, 0);
  column->setSpacing(0);
  column->addWidget(heading_);
  column->addLayout(rows, 1);

  SetListFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  connect(list_->verticalScrollBar(), &QScrollBar::valueChanged, gutter_,
          qOverload<>(&QWidget::update));
  connect(list_->horizontalScrollBar(), &QScrollBar::valueChanged, heading_,
          qOverload<>(&QWidget::update));
  connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
    // ListBox1DblClick(): a code line sets the PC.
    const int row = list_->row(item);
    if (row >= 0 && row < lines_.size() && is_listing_ && IsInstructionLine(lines_[row]))
      if (const auto address = LineAddress(lines_[row]))
        emit PcRequested(*address);
  });
}

void SourceView::SetListFont(const QFont& font) {
  list_->setFont(font);
  static_cast<HeadingWidget*>(heading_)->Update();
  gutter_->update();
}

void SourceView::SetListing(const QStringList& lines, bool is_listing) {
  lines_ = lines;
  is_listing_ = is_listing;
  find_index_ = 3;
  list_->clear();
  QStringList shown;
  shown.reserve(lines.size());
  for (const QString& line : lines)
    shown.append(ExpandTabs(line));
  list_->addItems(shown);
  gutter_->update();
}

void SourceView::SetBreakpointQuery(std::function<bool(uint32_t)> has_breakpoint) {
  has_breakpoint_ = std::move(has_breakpoint);
  gutter_->update();
}

void SourceView::RefreshBreakpoints() {
  gutter_->update();
}

int SourceView::RowAt(int y) const {
  const int vy = y - list_->viewport()->mapTo(this, QPoint(0, 0)).y();
  const QModelIndex index = list_->indexAt(QPoint(1, vy));
  return index.isValid() ? index.row() : -1;
}

std::optional<uint32_t> SourceView::CodeLineAddress(int row) const {
  if (!is_listing_ || row <= 2 || row >= lines_.size() || !IsInstructionLine(lines_[row]))
    return std::nullopt;
  return LineAddress(lines_[row]);
}

void SourceView::HighlightPc(uint32_t pc, bool halted) {
  if (!is_listing_)
    return;
  const QString address = QStringLiteral("%1").arg(pc, 8, 16, QLatin1Char('0')).toUpper();
  int found = -1;
  for (int i = 3; i < lines_.size(); i++) {
    if (lines_[i].left(8).toUpper() == address && (IsInstructionLine(lines_[i]) || halted))
      found = i;
  }
  if (found < 0)
    return;
  list_->setCurrentRow(found);
  // Not on screen: show it as the third visible row.
  const QRect r = list_->visualItemRect(list_->item(found));
  if (r.top() < 0 || r.bottom() > list_->viewport()->height())
    list_->scrollToItem(list_->item(std::max(found - 2, 0)), QAbstractItemView::PositionAtTop);
  gutter_->update();
}

bool SourceView::Find(const QString& text, bool next) {
  if (text.isEmpty())
    return false;
  if (!next)
    find_index_ = 3;
  if (find_index_ >= list_->count())
    return false;
  for (; find_index_ < list_->count(); ++find_index_) {
    if (list_->item(find_index_)->text().contains(text, Qt::CaseInsensitive)) {
      const int found = find_index_++;
      list_->setCurrentRow(found);
      const QRect r = list_->visualItemRect(list_->item(found));
      if (r.top() < 0 || r.bottom() > list_->viewport()->height())
        list_->scrollToItem(list_->item(std::max(found - 2, 0)), QAbstractItemView::PositionAtTop);
      gutter_->update();
      return true;
    }
  }
  return false;
}

std::optional<uint32_t> SourceView::SelectedAddress() const {
  return CodeLineAddress(list_->currentRow());
}

}  // namespace easym68k::gui
