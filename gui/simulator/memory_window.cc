// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "memory_window.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpressionValidator>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "easym68k/sim/types.h"

namespace easym68k::gui {

namespace {

constexpr int kSideWidth = 32;  // Panel2
constexpr uint32_t kMemSize = sim::kMemorySize;

QFont MemoryFont() {  // Memory1.dfm: Courier New, Height -13
  QFont font(QStringLiteral("Courier New"));
  font.setPixelSize(13);
  font.setStyleHint(QFont::TypeWriter);
  font.setFixedPitch(true);
  return font;
}

QLineEdit* HexEdit(QWidget* parent, int digits) {
  auto* e = new QLineEdit(parent);
  e->setMaxLength(digits);
  e->setValidator(new QRegularExpressionValidator(
      QRegularExpression(QStringLiteral("[0-9A-Fa-f]{0,%1}").arg(digits)), e));
  e->setFont(MemoryFont());
  e->setFixedWidth(
      QFontMetrics(e->font()).horizontalAdvance(QString(digits + 2, QLatin1Char('0'))));
  QObject::connect(e, &QLineEdit::textEdited, e, [e](const QString& t) {
    if (t != t.toUpper()) {
      const int c = e->cursorPosition();
      e->setText(t.toUpper());
      e->setCursorPosition(c);
    }
  });
  return e;
}

uint32_t HexValue(const QLineEdit* e) {
  return e->text().toUInt(nullptr, 16);
}

QToolButton* Arrow(QWidget* parent, Qt::ArrowType type) {
  auto* b = new QToolButton(parent);
  b->setArrowType(type);
  b->setAutoRepeat(true);
  return b;
}

}  // namespace

MemoryWindow::~MemoryWindow() {
  // The address field loses focus while QWidget destroys the children; its
  // editingFinished must not reach the already destroyed MemoryWindow part.
  address_->disconnect(this);
}

MemoryWindow::MemoryWindow(sim::Memory* memory, Access access, QWidget* parent)
    : QWidget(parent, Qt::Window), memory_(memory), access_(std::move(access)) {
  setWindowTitle(QStringLiteral("68000 Memory"));
  setMaximumWidth(640);
  setMinimumSize(200, 200);
  resize(640, 450);
  setFont(MemoryFont());
  setCursor(Qt::IBeamCursor);
  setFocusPolicy(Qt::ClickFocus);
  setAutoFillBackground(true);
  QPalette pal = palette();
  pal.setColor(QPalette::Window, Qt::white);
  setPalette(pal);
  notify_ = [this](const QString& text) {
    QMessageBox::information(this, QStringLiteral("Information"), text);
  };

  // Panel1: Address, From, To, Bytes, Save, Copy, Fill and the column heading.
  header_ = new QWidget(this);
  header_->setCursor(Qt::ArrowCursor);
  auto* grid = new QGridLayout(header_);
  grid->setContentsMargins(2, 2, 2, 0);
  grid->setVerticalSpacing(0);
  auto* fields = new QHBoxLayout;
  address_ = HexEdit(header_, 6);
  from_ = HexEdit(header_, 6);
  to_ = HexEdit(header_, 6);
  bytes_ = HexEdit(header_, 8);
  address_->setText(QStringLiteral("000000"));
  auto add = [&](const QString& label, QLineEdit* edit) {
    fields->addWidget(new QLabel(label, header_));
    fields->addWidget(edit);
  };
  add(QStringLiteral("Address: $"), address_);
  add(QStringLiteral("From: $"), from_);
  add(QStringLiteral("To: $"), to_);
  add(QStringLiteral("Bytes: $"), bytes_);
  auto* save = new QPushButton(QStringLiteral("Save"), header_);
  save->setToolTip(QStringLiteral("Save From..To to a file"));
  auto* copy = new QPushButton(QStringLiteral("Copy"), header_);
  auto* fill = new QPushButton(QStringLiteral("Fill"), header_);
  for (QPushButton* b : {save, copy, fill})
    fields->addWidget(b);
  fields->addStretch();
  grid->addLayout(fields, 0, 0);
  auto* heading = new QLabel(
      QStringLiteral("00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 0123456789ABCDEF"), header_);
  heading->setFont(font());
  heading->setContentsMargins(kHexLeft * ColumnWidth() - 2, 0, 0, 0);
  grid->addWidget(heading, 1, 0);
  connect(save, &QPushButton::clicked, this, &MemoryWindow::OnSave);
  connect(copy, &QPushButton::clicked, this, &MemoryWindow::Copy);
  connect(fill, &QPushButton::clicked, this, &MemoryWindow::Fill);
  connect(address_, &QLineEdit::textEdited, this, [this] {
    // Address1Change(): the row of the address, on a 16-byte boundary.
    mem_addr_ = HexValue(address_) & ~0xFu;
    update();
  });
  connect(address_, &QLineEdit::editingFinished, this, [this] { SetTop(mem_addr_); });
  address_->installEventFilter(this);

  // Panel2: Row and Page spin buttons, Live.
  side_ = new QWidget(this);
  side_->setCursor(Qt::ArrowCursor);
  auto* column = new QVBoxLayout(side_);
  column->setContentsMargins(2, 4, 2, 4);
  QFont small = font();
  small.setPixelSize(10);
  auto label = [&](const QString& text) {
    auto* l = new QLabel(text, side_);
    l->setFont(small);
    column->addWidget(l);
  };
  label(QStringLiteral("Row"));
  QToolButton* row_up = Arrow(side_, Qt::UpArrow);
  QToolButton* row_down = Arrow(side_, Qt::DownArrow);
  column->addWidget(row_up);
  column->addWidget(row_down);
  label(QStringLiteral("Page"));
  QToolButton* page_up = Arrow(side_, Qt::UpArrow);
  QToolButton* page_down = Arrow(side_, Qt::DownArrow);
  column->addWidget(page_up);
  column->addWidget(page_down);
  label(QStringLiteral("Live"));
  live_ = new QCheckBox(side_);
  column->addWidget(live_);
  column->addStretch();
  connect(row_up, &QToolButton::clicked, this, [this] { ScrollRows(-1); });
  connect(row_down, &QToolButton::clicked, this, [this] { ScrollRows(1); });
  connect(page_up, &QToolButton::clicked, this, [this] { ScrollRows(-Rows()); });
  connect(page_down, &QToolButton::clicked, this, [this] { ScrollRows(Rows()); });

  prompt_timer_.setInterval(500);
  connect(&prompt_timer_, &QTimer::timeout, this, [this] {
    prompt_visible_ = !prompt_visible_;
    update();
  });
  live_timer_.setInterval(100);
  connect(&live_timer_, &QTimer::timeout, this, [this] {
    if (live_->isChecked() && isVisible() && !Readable() && access_.request_snapshot) {
      requested_address_ = mem_addr_;
      requested_size_ = static_cast<uint32_t>(Rows()) * 16;
      access_.request_snapshot(requested_address_, requested_size_);
    }
  });
  live_timer_.start();
}

int MemoryWindow::RowHeight() const {
  return std::max(1, QFontMetrics(font()).height());
}
int MemoryWindow::ColumnWidth() const {
  return std::max(1, QFontMetrics(font()).horizontalAdvance(QLatin1Char('W')));
}
int MemoryWindow::HeaderHeight() const {
  return header_->sizeHint().height();
}

int MemoryWindow::Rows() const {
  return std::max(0, (height() - HeaderHeight()) / RowHeight());
}

void MemoryWindow::resizeEvent(QResizeEvent* event) {
  header_->setGeometry(0, 0, width() - kSideWidth, HeaderHeight());
  side_->setGeometry(width() - kSideWidth, HeaderHeight(), kSideWidth, height() - HeaderHeight());
  QWidget::resizeEvent(event);
}

void MemoryWindow::ShowAddress(uint32_t address) {
  SetTop(address & sim::kAddressMask);
}

void MemoryWindow::Refresh() {
  snapshot_.clear();
  update();
}

void MemoryWindow::OnSnapshot(uint32_t address, const QByteArray& bytes) {
  // Copies requested by other windows are for them.
  if (address != requested_address_ || static_cast<uint32_t>(bytes.size()) != requested_size_)
    return;
  snap_address_ = address;
  snapshot_ = bytes;
  update();
}

// RowSpin/PageSpin: the top row stays within memory.
void MemoryWindow::SetTop(int64_t address) {
  if (address < 0)
    address = 0;
  if (address >= kMemSize)
    address = kMemSize - 16;
  mem_addr_ = static_cast<uint32_t>(address) & ~0xFu;
  address_->setText(QStringLiteral("%1").arg(mem_addr_, 6, 16, QLatin1Char('0')).toUpper());
  update();
}

void MemoryWindow::ScrollRows(int rows) {
  SetTop(static_cast<int64_t>(mem_addr_) + rows * 16);
}

bool MemoryWindow::ByteAt(uint32_t address, uint8_t* value) const {
  if (Readable()) {
    *value = memory_->RawData()[address];
    return true;
  }
  if (address >= snap_address_ &&
      address - snap_address_ < static_cast<uint32_t>(snapshot_.size())) {
    *value = static_cast<uint8_t>(snapshot_[static_cast<qsizetype>(address - snap_address_)]);
    return true;
  }
  return false;
}

QString MemoryWindow::RowText(int row) const {
  const uint32_t addr = mem_addr_ + static_cast<uint32_t>(row) * 16;
  QString text = QStringLiteral("%1: ").arg(addr, 8, 16, QLatin1Char('0')).toUpper();
  QString ascii;
  for (uint32_t i = 0; i < 16; i++) {
    uint8_t b = 0;
    if (addr + i >= kMemSize || !ByteAt(addr + i, &b)) {
      text += QStringLiteral("xx ");
      ascii += QLatin1Char('-');
    } else {
      text += QStringLiteral("%1 ").arg(b, 2, 16, QLatin1Char('0')).toUpper();
      // The original's signed char test: $00-$1F and $80-$FF show as '-'.
      ascii += (b >= 0x20 && b < 0x80) ? QChar(b) : QLatin1Char('-');
    }
  }
  return text + ascii;
}

// Memory1.cpp FormPaint().
void MemoryWindow::paintEvent(QPaintEvent*) {
  QPainter out(this);
  const int top = HeaderHeight() + 1;
  const int rows = Rows();
  const uint32_t first = mem_addr_;
  const uint32_t last = mem_addr_ + static_cast<uint32_t>(rows) * 16;
  uint8_t probe = 0;
  const bool available =
      Readable() ||
      (ByteAt(first, &probe) && (last <= first || ByteAt(std::min(last, kMemSize) - 1, &probe)));
  if (!available) {
    out.drawPixmap(0, top, last_image_);
  } else {
    QPixmap image(width() - kSideWidth, rows * RowHeight());
    image.fill(Qt::white);
    QPainter p(&image);
    p.setFont(font());
    const QFontMetrics fm(font());
    const int cw = ColumnWidth();
    for (int r = 0; r < rows; r++) {
      const QString text = RowText(r);
      const uint32_t addr = mem_addr_ + static_cast<uint32_t>(r) * 16;
      const int y = r * RowHeight() + fm.ascent();
      for (int c = 0; c < text.size(); c++) {
        bool invalid = addr >= kMemSize;
        if (c >= kHexLeft && c < kAscLeft)
          invalid = addr + static_cast<uint32_t>((c - kHexLeft) / 3) >= kMemSize;
        else if (c >= kAscLeft)
          invalid = addr + static_cast<uint32_t>(c - kAscLeft) >= kMemSize;
        p.setPen(invalid ? Qt::red : Qt::black);
        p.drawText(c * cw, y, QString(text.at(c)));
      }
    }
    p.end();
    last_image_ = image;
    out.drawPixmap(0, top, image);
  }
  // drawPrompt(): a flashing frame around the cursor cell.
  if (cursor_shown_ && prompt_visible_ && hasFocus()) {
    const int cw = ColumnWidth();
    const int rh = RowHeight();
    out.setPen(Qt::black);
    out.setBrush(Qt::NoBrush);
    out.drawRect(QRect(col_ * cw, top + row_ * rh, cw, rh - 1));
  }
}

// Memory1.cpp gotoRC(): keeps the cursor on a digit or character; leaving
// the top or bottom row scrolls.
void MemoryWindow::GotoRC(int row, int col) {
  row_ = row;
  col_ = col;
  if (row_ < 0) {
    row_ = 0;
    ScrollRows(-1);
  } else if (row_ > Rows() - 1) {
    row_ = std::max(0, Rows() - 1);
    ScrollRows(1);
  }
  if (col_ < kHexLeft)
    col_ = kHexLeft;
  else if (col_ < kAscLeft) {
    if (col_ % 3 == 0)
      col_++;
  } else if (col_ > kAscRight) {
    col_ = kAscRight;
  }
  prompt_visible_ = true;
  update();
}

void MemoryWindow::PlaceCursor(int row, int col) {
  cursor_shown_ = true;
  setFocus();
  GotoRC(row, col);
  prompt_timer_.start();
}

void MemoryWindow::mousePressEvent(QMouseEvent* event) {
  const QPoint p = event->position().toPoint();
  if (p.y() <= HeaderHeight())
    return;
  PlaceCursor((p.y() - HeaderHeight() - 1) / RowHeight(), p.x() / ColumnWidth());
}

// Memory1.cpp FormKeyPress(): a hex digit changes the nybble under the
// cursor, any character in the ASCII area the byte; the cursor advances.
void MemoryWindow::TypeKey(char key) {
  if (!cursor_shown_)
    return;
  uint32_t addr = mem_addr_ + static_cast<uint32_t>(row_) * 16;
  bool hex = false;
  bool low = false;
  if (col_ <= kHexRight) {
    addr += static_cast<uint32_t>((col_ - kHexLeft) / 3);
    low = (col_ - kHexLeft) % 3 != 0;
    hex = true;
  } else {
    addr += static_cast<uint32_t>(col_ - kAscLeft);
  }
  if (addr >= kMemSize || !Readable()) {
    QApplication::beep();
    return;
  }
  uint8_t* m = memory_->RawData();
  if (hex) {
    const char k = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
    uint8_t data = 0;
    if (k >= '0' && k <= '9') {
      data = static_cast<uint8_t>(k - '0');
    } else if (k >= 'A' && k <= 'F') {
      data = static_cast<uint8_t>(k - 'A' + 10);
    } else {
      QApplication::beep();
      return;
    }
    m[addr] = low ? static_cast<uint8_t>((m[addr] & 0xF0) | data)
                  : static_cast<uint8_t>((m[addr] & 0x0F) | (data << 4));
    int col = col_ + 1;
    int row = row_;
    if (col > kHexRight) {
      col = kHexLeft;
      row++;
    }
    GotoRC(row, col);
  } else {
    m[addr] = static_cast<uint8_t>(key);
    int col = col_ + 1;
    int row = row_;
    if (col > kAscRight) {
      col = kAscLeft;
      row++;
    }
    GotoRC(row, col);
  }
  emit MemoryEdited(addr);
  update();
}

// Memory1.cpp FormKeyDown() / FormKeyPress().
void MemoryWindow::keyPressEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_PageUp:
      ScrollRows(-Rows());
      return;
    case Qt::Key_PageDown:
      ScrollRows(Rows());
      return;
    case Qt::Key_Up:
      GotoRC(row_ - 1, col_);
      return;
    case Qt::Key_Down:
      GotoRC(row_ + 1, col_);
      return;
    case Qt::Key_Left: {
      int col = col_ - 1;
      if (col < kAscLeft && col % 3 == 0)
        col--;
      GotoRC(row_, col);
      return;
    }
    case Qt::Key_Right:
      GotoRC(row_, col_ + 1);
      return;
    case Qt::Key_Tab:
      if (event->modifiers() & Qt::ControlModifier) {
        emit NextWindowRequested();
        return;
      }
      break;
    default:
      break;
  }
  const QString text = event->text();
  if (text.size() == 1 && text.at(0).unicode() < 0x100) {
    TypeKey(static_cast<char>(text.at(0).unicode()));
    return;
  }
  QWidget::keyPressEvent(event);
}

void MemoryWindow::wheelEvent(QWheelEvent* event) {
  ScrollRows(event->angleDelta().y() < 0 ? 1 : -1);
  event->accept();
}

// Address1KeyDown(): Page/arrow keys scroll, Ctrl+Tab goes on.
bool MemoryWindow::eventFilter(QObject* watched, QEvent* event) {
  if (watched == address_ && event->type() == QEvent::KeyPress) {
    auto* key = static_cast<QKeyEvent*>(event);
    switch (key->key()) {
      case Qt::Key_PageUp:
        ScrollRows(-Rows());
        return true;
      case Qt::Key_PageDown:
        ScrollRows(Rows());
        return true;
      case Qt::Key_Up:
        ScrollRows(-1);
        return true;
      case Qt::Key_Down:
        ScrollRows(1);
        return true;
      case Qt::Key_Tab:
        if (key->modifiers() & Qt::ControlModifier) {
          emit NextWindowRequested();
          return true;
        }
        break;
      default:
        break;
    }
  }
  return QWidget::eventFilter(watched, event);
}

// Memory1.cpp CopyClick(): Bytes bytes from From to To, overlapping ranges
// copied correctly. The whole range is checked first (the original stopped
// half-way through a downward copy).
void MemoryWindow::Copy() {
  if (!Readable()) {
    QApplication::beep();
    return;
  }
  const uint32_t from = HexValue(from_);
  const uint32_t to = HexValue(to_);
  const uint32_t bytes = HexValue(bytes_);
  if (to == from || bytes == 0 || static_cast<uint64_t>(from) + bytes > kMemSize ||
      static_cast<uint64_t>(to) + bytes > kMemSize) {
    notify_(QStringLiteral("Invalid copy range"));
    return;
  }
  std::memmove(memory_->RawData() + to, memory_->RawData() + from, bytes);
  emit MemoryEdited(to);
  update();
}

// Memory1.cpp FillClick(): From..To filled with the 4-byte value in Bytes
// (big-endian, repeated). A range past memory is refused before filling
// (the original filled up to the end first).
void MemoryWindow::Fill() {
  if (!Readable()) {
    QApplication::beep();
    return;
  }
  const uint32_t from = HexValue(from_);
  const uint32_t to = HexValue(to_);
  const uint32_t value = HexValue(bytes_);
  if (from > to)
    return;
  if (to >= kMemSize) {
    notify_(QStringLiteral("Invalid memory range"));
    return;
  }
  const uint8_t data[4] = {static_cast<uint8_t>(value >> 24), static_cast<uint8_t>(value >> 16),
                           static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value)};
  for (uint32_t i = from; i <= to; i++)
    memory_->RawData()[i] = data[(i - from) % 4];
  emit MemoryEdited(from);
  update();
}

// Memory1.cpp SaveClick(): From..To to a binary file.
bool MemoryWindow::SaveTo(const QString& path) {
  const uint32_t from = HexValue(from_);
  const uint32_t to = HexValue(to_);
  if (to < from || to >= kMemSize) {
    QApplication::beep();
    notify_(QStringLiteral("Invalid memory range."));
    return false;
  }
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly) ||
      file.write(reinterpret_cast<const char*>(memory_->RawData() + from), to - from + 1) !=
          static_cast<qint64>(to - from + 1)) {
    notify_(QStringLiteral("Error saving 68K memory to file."));
    return false;
  }
  return true;
}

void MemoryWindow::OnSave() {
  const uint32_t from = HexValue(from_);
  const uint32_t to = HexValue(to_);
  if (to < from || to >= kMemSize) {
    QApplication::beep();
    notify_(QStringLiteral("Invalid memory range."));
    return;
  }
  QFileDialog dialog(this, QStringLiteral("Save"));
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(QStringLiteral("bin"));  // SaveDialog DefaultExt
  if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty())
    SaveTo(dialog.selectedFiles().first());
}

}  // namespace easym68k::gui
