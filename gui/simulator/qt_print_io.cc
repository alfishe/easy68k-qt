// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "qt_print_io.h"

#include <algorithm>
#include <QFontMetrics>
#include <utility>

#include "gui_thread.h"

namespace easym68k::gui {

QFont QtPrintIO::DefaultFont() {
  QFont font(QStringLiteral("Courier New"));  // FontDialogPrinter: Height -13 = 10 pt
  font.setPointSize(10);
  font.setStyleHint(QFont::TypeWriter);
  font.setFixedPitch(true);
  return font;
}

QtPrintIO::QtPrintIO(QObject* gui_context, DeviceFactory factory, const QFont& font)
    : receiver_(std::make_unique<QObject>()), factory_(std::move(factory)), font_(font) {
  receiver_->moveToThread(gui_context->thread());
}

QtPrintIO::~QtPrintIO() {
  receiver_.reset();  // drops queued characters
  FinishDocument();
}

void QtPrintIO::PrintChar(char ch) {
  PostToThread(receiver_.get(), [this, ch] { DoPrintChar(ch); });
}

void QtPrintIO::FormFeed() {
  PostToThread(receiver_.get(), [this] { DoFormFeed(); });
}

void QtPrintIO::EndJob() {
  PostToThread(receiver_.get(), [this] { FinishDocument(); });
}

void QtPrintIO::SetFont(const QFont& font) {
  PostToThread(receiver_.get(), [this, font] { font_ = font; });
}

// CODE9.CPP initPrint() + BeginDoc.
bool QtPrintIO::InitPrint() {
  device_ = factory_ ? factory_() : nullptr;
  if (!device_ || !painter_.begin(device_.get())) {
    device_.reset();
    return false;
  }
  painter_.setFont(font_);
  const QFontMetrics fm(font_, device_.get());
  row_height_ = fm.height();
  col_width_ = fm.horizontalAdvance(QLatin1Char('W'));
  ascent_ = fm.ascent();
  top_ = device_->height() / 11;
  bottom_ = device_->height() - top_;
  left_ = device_->width() / 8;
  right_ = device_->width() - left_;
  x_ = carry_ ? carry_x_ : left_;
  y_ = carry_ ? carry_y_ : top_;
  carry_ = false;
  printing_ = true;
  return true;
}

// EndDoc: the page is printed.
void QtPrintIO::FinishDocument() {
  if (!printing_)
    return;
  painter_.end();
  device_.reset();
  printing_ = false;
}

// CODE9.CPP formFeed(): without an open job a blank page is printed.
void QtPrintIO::DoFormFeed() {
  if (!printing_ && !InitPrint())
    return;
  FinishDocument();
  y_ = top_;
}

void QtPrintIO::LineFeed() {
  y_ += row_height_;
  if (y_ > bottom_) {
    carry_ = true;
    carry_x_ = x_;
    carry_y_ = top_;
    FinishDocument();
  }
}

void QtPrintIO::NextX() {
  x_ += col_width_;
  if (x_ > right_) {
    x_ = x_ - right_ + left_;
    LineFeed();
  }
}

// CODE9.CPP printChar(). Characters $80-$FF are printed (the original's
// signed `ch >= ' '` test dropped them).
void QtPrintIO::DoPrintChar(char ch) {
  if (!printing_ && !InitPrint())
    return;  // no printer
  switch (ch) {
    case '\b':
      x_ = std::max(x_ - col_width_, left_);
      break;
    case '\f':
      DoFormFeed();
      break;
    case '\n':
      LineFeed();
      break;
    case '\r':
      x_ = left_;
      break;
    case '\t':
      x_ += 4 * col_width_;
      NextX();
      break;
    case '\v':
      y_ += 4 * row_height_;
      if (y_ > bottom_) {
        carry_ = true;
        carry_x_ = x_;
        carry_y_ = y_ - bottom_ + top_;
        FinishDocument();
      }
      break;
    default:
      if (static_cast<unsigned char>(ch) >= ' ') {
        painter_.drawText(x_, y_ + ascent_, QString(QChar(static_cast<unsigned char>(ch))));
        NextX();
      }
      break;
  }
}

}  // namespace easym68k::gui
