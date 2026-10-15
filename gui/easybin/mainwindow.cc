// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easybin/mainwindow.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTextStream>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "easym68k/sim/types.h"

namespace easym68k::easybin {

namespace {

constexpr int kSideWidth = 32;
constexpr uint32_t kMemSize = sim::kMemorySize;

QFont EasyBinFont() {
  QFont font(QStringLiteral("Courier New"));
  font.setPixelSize(13);
  font.setStyleHint(QFont::TypeWriter);
  font.setFixedPitch(true);
  return font;
}

QLineEdit* HexEdit(QWidget* parent, int digits, const QString& initial) {
  auto* e = new QLineEdit(parent);
  e->setMaxLength(digits);
  e->setValidator(new QRegularExpressionValidator(
      QRegularExpression(QStringLiteral("[0-9A-Fa-f]{0,%1}").arg(digits)), e));
  e->setFont(EasyBinFont());
  e->setFixedWidth(
      QFontMetrics(e->font()).horizontalAdvance(QString(digits + 2, QLatin1Char('0'))));
  e->setText(initial);
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

QColor SplitColor(int index) {
  switch (index) {
    case 0:
      return Qt::black;
    case 1:
      return QColor(0x80, 0x80, 0x00);  // clOlive
    case 2:
      return QColor(0x00, 0x80, 0x00);  // clGreen
    case 3:
      return QColor(0x00, 0x00, 0xFF);  // clBlue
    default:
      return Qt::black;
  }
}

bool ParseHexByte(const QString& s, int pos, uint8_t* out) {
  if (pos + 2 > s.size())
    return false;
  bool ok = false;
  const uint v = s.mid(pos, 2).toUInt(&ok, 16);
  if (!ok)
    return false;
  *out = static_cast<uint8_t>(v & 0xFFu);
  return true;
}

bool ParseHexUInt(const QString& s, int pos, int digits, uint32_t* out) {
  if (pos + digits > s.size())
    return false;
  bool ok = false;
  const uint32_t v = s.mid(pos, digits).toUInt(&ok, 16);
  if (!ok)
    return false;
  *out = v;
  return true;
}

QString FinishSRecord(const QString& type_and_payload, int byte_count) {
  // type_and_payload is "S<type><address_and_data_hex>" without the 2-digit
  // byte count or checksum.
  const QString body = QStringLiteral("%1%2")
                           .arg(byte_count & 0xFF, 2, 16, QLatin1Char('0'))
                           .arg(type_and_payload.mid(2))
                           .toUpper();
  uint8_t sum = 0;
  for (int i = 0; i + 2 <= body.size(); i += 2) {
    uint8_t b = 0;
    if (ParseHexByte(body, i, &b)) {
      sum = static_cast<uint8_t>(sum + b);
    }
  }
  const uint8_t checksum = static_cast<uint8_t>(~sum & 0xFFu);
  return QStringLiteral("%1%2%3\n")
      .arg(type_and_payload.left(2), body)
      .arg(checksum, 2, 16, QLatin1Char('0'))
      .toUpper();
}

}  // namespace

MainWindow::~MainWindow() {
  address_->disconnect(this);
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  memory_.Erase();  // erase 68000 memory to $FF

  setWindowTitle(QString::fromLatin1(kTitle));
  setMinimumSize(640, 440);
  setMaximumWidth(640);
  resize(640, 480);
  setFont(EasyBinFont());
  setCursor(Qt::IBeamCursor);
  setFocusPolicy(Qt::ClickFocus);
  setAcceptDrops(true);
  setAutoFillBackground(true);
  QPalette pal = palette();
  pal.setColor(QPalette::Window, Qt::white);
  setPalette(pal);

  notify_ = [this](const QString& text) {
    QMessageBox::information(this, QStringLiteral("Information"), text);
  };

  // Menu bar (Form1u.dfm MainMenu1 / ActionList1).
  QMenu* file_menu = menuBar()->addMenu(QStringLiteral("&File"));
  new_action_ = file_menu->addAction(QStringLiteral("&New"));
  file_menu->addSeparator();
  open_srec_action_ = file_menu->addAction(QStringLiteral("&Open S-Record..."));
  open_srec_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
  open_bin_action_ = file_menu->addAction(QStringLiteral("Open &Binary..."));
  open_bin_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
  file_menu->addSeparator();
  save_srec_action_ = file_menu->addAction(QStringLiteral("Save S-&Record..."));
  save_srec_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
  save_bin_action_ = file_menu->addAction(QStringLiteral("&Save Binary..."));
  save_bin_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
  file_menu->addSeparator();
  exit_action_ = file_menu->addAction(QStringLiteral("E&xit"));

  QMenu* help_menu = menuBar()->addMenu(QStringLiteral("&Help"));
  help_action_ = help_menu->addAction(QStringLiteral("&EASyBIN Help"));
  help_action_->setShortcut(QKeySequence(Qt::Key_F1));
  about_action_ = help_menu->addAction(QStringLiteral("&About EASyBIN"));

  connect(new_action_, &QAction::triggered, this, [this] { NewEnvironment(/*confirm=*/true); });
  connect(open_srec_action_, &QAction::triggered, this, &MainWindow::OnOpenSRecord);
  connect(open_bin_action_, &QAction::triggered, this, &MainWindow::OnOpenBinary);
  connect(save_srec_action_, &QAction::triggered, this, &MainWindow::OnSaveSRecord);
  connect(save_bin_action_, &QAction::triggered, this, &MainWindow::OnSaveBinary);
  connect(exit_action_, &QAction::triggered, this, &QWidget::close);
  connect(help_action_, &QAction::triggered, this, &MainWindow::OnHelp);
  connect(about_action_, &QAction::triggered, this, &MainWindow::OnAbout);

  // Panel1: top controls + memory range toolbar + column heading.
  header_ = new QWidget(this);
  header_->setCursor(Qt::ArrowCursor);
  auto* header_layout = new QVBoxLayout(header_);
  header_layout->setContentsMargins(4, 2, 4, 0);
  header_layout->setSpacing(2);

  // Top row: three group boxes.
  auto* groups_row = new QHBoxLayout();
  groups_row->setSpacing(6);

  // Group 1: S-Record File
  auto* s68_group = new QGroupBox(QStringLiteral(" S-Record File "), header_);
  auto* s68_grid = new QGridLayout(s68_group);
  s68_grid->setContentsMargins(6, 4, 6, 4);
  s68_grid->setSpacing(2);
  start_address_ = HexEdit(s68_group, 8, QStringLiteral("00000000"));
  s68_end_address_ = new QLabel(QStringLiteral("000000"), s68_group);
  s68_end_address_->setFont(EasyBinFont());
  s68_length_ = new QLabel(QStringLiteral("000000"), s68_group);
  s68_length_->setFont(EasyBinFont());
  open_srec_btn_ = new QPushButton(QStringLiteral("Open"), s68_group);
  connect(open_srec_btn_, &QPushButton::clicked, this, &MainWindow::OnOpenSRecord);

  s68_grid->addWidget(new QLabel(QStringLiteral("Start Address: $"), s68_group), 0, 0,
                      Qt::AlignRight);
  s68_grid->addWidget(start_address_, 0, 1);
  s68_grid->addWidget(new QLabel(QStringLiteral("Last Address: $00"), s68_group), 1, 0,
                      Qt::AlignRight);
  s68_grid->addWidget(s68_end_address_, 1, 1);
  auto* len_row = new QHBoxLayout();
  len_row->addWidget(open_srec_btn_);
  len_row->addStretch();
  len_row->addWidget(new QLabel(QStringLiteral("Length: $"), s68_group));
  s68_grid->addLayout(len_row, 2, 0);
  s68_grid->addWidget(s68_length_, 2, 1);
  groups_row->addWidget(s68_group);

  // Group 2: Binary Input/Output Split
  auto* split_box = new QGroupBox(QStringLiteral(" Binary Input/Output Split "), header_);
  auto* split_vbox = new QVBoxLayout(split_box);
  split_vbox->setContentsMargins(6, 4, 6, 4);
  split_vbox->setSpacing(2);
  split_group_ = new QButtonGroup(this);
  split_1_radio_ = new QRadioButton(QStringLiteral("1 File - All Bytes"), split_box);
  split_2_radio_ = new QRadioButton(QStringLiteral("2 Files - Every 2nd Byte"), split_box);
  split_4_radio_ = new QRadioButton(QStringLiteral("4 Files - Every 4th Byte"), split_box);
  split_1_radio_->setChecked(true);
  split_group_->addButton(split_1_radio_, 0);
  split_group_->addButton(split_2_radio_, 1);
  split_group_->addButton(split_4_radio_, 2);
  split_vbox->addWidget(split_1_radio_);
  split_vbox->addWidget(split_2_radio_);
  split_vbox->addWidget(split_4_radio_);
  connect(split_group_, &QButtonGroup::idClicked, this, &MainWindow::SetSplitIndex);
  groups_row->addWidget(split_box);

  // Group 3: Binary File(s)
  auto* out_group = new QGroupBox(QStringLiteral(" Binary File(s) "), header_);
  auto* out_grid = new QGridLayout(out_group);
  out_grid->setContentsMargins(6, 4, 6, 4);
  out_grid->setSpacing(2);
  output_first_address_ = HexEdit(out_group, 6, QStringLiteral("000000"));
  output_length_ = HexEdit(out_group, 6, QStringLiteral("000000"));
  save_bin_btn_ = new QPushButton(QStringLiteral("Save"), out_group);
  connect(save_bin_btn_, &QPushButton::clicked, this, &MainWindow::OnSaveBinary);
  connect(output_first_address_, &QLineEdit::textEdited, this, [this] { update(); });
  connect(output_length_, &QLineEdit::textEdited, this, [this] { update(); });

  out_grid->addWidget(new QLabel(QStringLiteral("First Address: $00"), out_group), 0, 0,
                      Qt::AlignRight);
  out_grid->addWidget(output_first_address_, 0, 1);
  out_grid->addWidget(new QLabel(QStringLiteral("Length: $"), out_group), 1, 0, Qt::AlignRight);
  out_grid->addWidget(output_length_, 1, 1);

  auto* split_lbl_row = new QHBoxLayout();
  split_lbl_row->addWidget(save_bin_btn_);
  split_lbl_row->addStretch();
  split_lbl_row->addWidget(new QLabel(QStringLiteral("File Split:"), out_group));
  out_grid->addLayout(split_lbl_row, 2, 0);

  auto* split_nums = new QHBoxLayout();
  for (int i = 0; i < 4; ++i) {
    auto* lbl = new QLabel(QString::number(i), out_group);
    QFont f = lbl->font();
    f.setBold(true);
    lbl->setFont(f);
    QPalette p = lbl->palette();
    p.setColor(QPalette::WindowText, SplitColor(i));
    lbl->setPalette(p);
    lbl->setVisible(i == 0);
    split_labels_[i] = lbl;
    split_nums->addWidget(lbl);
  }
  split_nums->addStretch();
  out_grid->addLayout(split_nums, 2, 1);
  groups_row->addWidget(out_group);
  header_layout->addLayout(groups_row);

  // Second row: Address, From, To, Size + Copy, FillByte + Fill.
  auto* tools_row = new QHBoxLayout();
  tools_row->setSpacing(4);
  address_ = HexEdit(header_, 6, QStringLiteral("000000"));
  from_ = HexEdit(header_, 6, QStringLiteral("000000"));
  to_ = HexEdit(header_, 6, QStringLiteral("000000"));
  bytes_ = HexEdit(header_, 6, QStringLiteral("000000"));
  fill_byte_ = HexEdit(header_, 2, QStringLiteral("FF"));
  copy_btn_ = new QPushButton(QStringLiteral("Copy"), header_);
  fill_btn_ = new QPushButton(QStringLiteral("Fill"), header_);

  tools_row->addWidget(new QLabel(QStringLiteral("Address: $00"), header_));
  tools_row->addWidget(address_);
  tools_row->addWidget(new QLabel(QStringLiteral("From: $00"), header_));
  tools_row->addWidget(from_);
  tools_row->addWidget(new QLabel(QStringLiteral("To: $00"), header_));
  tools_row->addWidget(to_);
  tools_row->addWidget(new QLabel(QStringLiteral("Size: $"), header_));
  tools_row->addWidget(bytes_);
  tools_row->addWidget(copy_btn_);
  tools_row->addWidget(new QLabel(QStringLiteral("$"), header_));
  tools_row->addWidget(fill_byte_);
  tools_row->addWidget(fill_btn_);
  tools_row->addStretch();
  header_layout->addLayout(tools_row);

  connect(copy_btn_, &QPushButton::clicked, this, &MainWindow::CopyRange);
  connect(fill_btn_, &QPushButton::clicked, this, &MainWindow::FillRange);
  connect(address_, &QLineEdit::textEdited, this, [this] {
    mem_addr_ = HexValue(address_) & ~0xFu;
    update();
  });
  connect(address_, &QLineEdit::editingFinished, this, [this] { SetTop(mem_addr_); });

  for (QLineEdit* edit : {start_address_, output_first_address_, output_length_, address_, from_,
                          to_, bytes_, fill_byte_}) {
    edit->installEventFilter(this);
  }

  // Third row: hex + ASCII column header.
  auto* heading = new QLabel(QStringLiteral("00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F "
                                            "0123456789ABCDEF"),
                             header_);
  heading->setFont(font());
  heading->setContentsMargins(kHexLeft * ColumnWidth() - 4, 0, 0, 0);
  header_layout->addWidget(heading);

  // Panel2: Row and Page spin buttons on the right.
  side_ = new QWidget(this);
  side_->setCursor(Qt::ArrowCursor);
  auto* side_col = new QVBoxLayout(side_);
  side_col->setContentsMargins(2, 4, 2, 4);
  QFont small = font();
  small.setPixelSize(10);
  auto add_side_label = [&](const QString& text) {
    auto* l = new QLabel(text, side_);
    l->setFont(small);
    side_col->addWidget(l);
  };
  add_side_label(QStringLiteral("Row"));
  QToolButton* row_up = Arrow(side_, Qt::UpArrow);
  QToolButton* row_down = Arrow(side_, Qt::DownArrow);
  side_col->addWidget(row_up);
  side_col->addWidget(row_down);
  add_side_label(QStringLiteral("Page"));
  QToolButton* page_up = Arrow(side_, Qt::UpArrow);
  QToolButton* page_down = Arrow(side_, Qt::DownArrow);
  side_col->addWidget(page_up);
  side_col->addWidget(page_down);
  side_col->addStretch();

  connect(row_up, &QToolButton::clicked, this, [this] { ScrollRows(-1); });
  connect(row_down, &QToolButton::clicked, this, [this] { ScrollRows(1); });
  connect(page_up, &QToolButton::clicked, this, [this] { ScrollRows(-Rows()); });
  connect(page_down, &QToolButton::clicked, this, [this] { ScrollRows(Rows()); });

  prompt_timer_.setInterval(500);
  connect(&prompt_timer_, &QTimer::timeout, this, [this] {
    prompt_visible_ = !prompt_visible_;
    update();
  });
}

void MainWindow::Notify(const QString& message) {
  notifications_.push_back(message);
  if (notify_) {
    notify_(message);
  }
}

void MainWindow::ApplyArguments(const QStringList& args) {
  if (!args.isEmpty() && QFileInfo::exists(args.first())) {
    OpenSRecordFile(args.first());
  }
}

int MainWindow::RowHeight() const {
  return std::max(1, QFontMetrics(font()).height());
}

int MainWindow::ColumnWidth() const {
  return std::max(1, QFontMetrics(font()).horizontalAdvance(QLatin1Char('W')));
}

int MainWindow::HeaderTop() const {
  return menuBar()->sizeHint().height();
}

int MainWindow::HeaderHeight() const {
  return header_->sizeHint().height();
}

int MainWindow::CanvasTop() const {
  return HeaderTop() + HeaderHeight() + 1;
}

int MainWindow::Rows() const {
  return std::max(0, (height() - CanvasTop()) / RowHeight());
}

void MainWindow::resizeEvent(QResizeEvent* event) {
  const int top = HeaderTop();
  const int hh = HeaderHeight();
  header_->setGeometry(0, top, width() - kSideWidth, hh);
  side_->setGeometry(width() - kSideWidth, top + hh, kSideWidth, height() - (top + hh));
  QMainWindow::resizeEvent(event);
}

QLabel* MainWindow::SplitLabel(int index) const {
  if (index < 0 || index >= 4)
    return nullptr;
  return split_labels_[index];
}

void MainWindow::SetSplitIndex(int index) {
  if (index < 0 || index > 2)
    return;
  if (split_group_->button(index) && !split_group_->button(index)->isChecked()) {
    split_group_->button(index)->setChecked(true);
  }
  split_ = index << 1;  // 0, 2, or 4
  split_labels_[0]->setVisible(true);
  split_labels_[1]->setVisible(split_ >= 2);
  split_labels_[2]->setVisible(split_ >= 4);
  split_labels_[3]->setVisible(split_ >= 4);
  update();
}

void MainWindow::ShowAddress(uint32_t address) {
  SetTop(address & sim::kAddressMask);
}

void MainWindow::SetTop(int64_t address) {
  if (address < 0)
    address = 0;
  if (address >= kMemSize)
    address = kMemSize - 16;
  mem_addr_ = static_cast<uint32_t>(address) & ~0xFu;
  address_->setText(QStringLiteral("%1").arg(mem_addr_, 6, 16, QLatin1Char('0')).toUpper());
  update();
}

void MainWindow::ScrollRows(int rows) {
  SetTop(static_cast<int64_t>(mem_addr_) + static_cast<int64_t>(rows) * 16);
}

void MainWindow::ErasePrompt() {
  prompt_timer_.stop();
  prompt_visible_ = false;
  update();
}

void MainWindow::GotoRC(int row, int col) {
  row_ = row;
  col_ = col;
  if (row_ < 0) {
    row_ = 0;
    ScrollRows(-1);
  } else if (row_ > Rows() - 1) {
    row_ = std::max(0, Rows() - 1);
    ScrollRows(1);
  }
  if (col_ < kHexLeft) {
    col_ = kHexLeft;
  } else if (col_ < kAscLeft) {
    if (col_ % 3 == 0)
      col_++;
  } else if (col_ > kAscRight) {
    col_ = kAscRight;
  }
  prompt_visible_ = true;
  update();
}

void MainWindow::PlaceCursor(int row, int col) {
  cursor_shown_ = true;
  setFocus();
  GotoRC(row, col);
  prompt_timer_.start();
}

QColor MainWindow::ByteColor(uint32_t address) const {
  if (address >= kMemSize)
    return Qt::red;
  const uint32_t out_first = HexValue(output_first_address_);
  const uint32_t out_length = HexValue(output_length_);
  if (out_length == 0)
    return Qt::gray;
  const uint64_t out_last =
      static_cast<uint64_t>(out_first) + static_cast<uint64_t>(out_length) - 1;
  if (address < out_first || address > out_last)
    return Qt::gray;
  if (split_ == 0)
    return Qt::black;
  return SplitColor(static_cast<int>((address - out_first) % split_));
}

QString MainWindow::RowText(int row) const {
  const uint32_t addr = mem_addr_ + static_cast<uint32_t>(row) * 16;
  QString text = QStringLiteral("%1: ").arg(addr, 8, 16, QLatin1Char('0')).toUpper();
  QString ascii;
  const uint8_t* raw = memory_.RawData();
  for (uint32_t i = 0; i < 16; ++i) {
    if (addr + i >= kMemSize) {
      text += QStringLiteral("xx ");
      ascii += QLatin1Char('-');
    } else {
      const uint8_t b = raw[addr + i];
      text += QStringLiteral("%1 ").arg(b, 2, 16, QLatin1Char('0')).toUpper();
      ascii += (b >= 0x20 && b < 0x80) ? QChar(b) : QLatin1Char('-');
    }
  }
  return text + ascii;
}

void MainWindow::paintEvent(QPaintEvent*) {
  QPainter out(this);
  const int top = CanvasTop();
  const int rows = Rows();
  if (rows <= 0)
    return;

  QPixmap image(width() - kSideWidth, rows * RowHeight());
  image.fill(Qt::white);
  QPainter p(&image);
  p.setFont(font());
  const QFontMetrics fm(font());
  const int cw = ColumnWidth();
  const int rh = RowHeight();

  for (int r = 0; r < rows; ++r) {
    const QString text = RowText(r);
    const uint32_t addr = mem_addr_ + static_cast<uint32_t>(r) * 16;
    const int y = r * rh + fm.ascent();
    for (int c = 0; c < text.size(); ++c) {
      QColor color = (addr >= kMemSize) ? Qt::red : Qt::black;
      if (c >= kHexLeft && c < kAscLeft) {
        const uint32_t byte_addr = addr + static_cast<uint32_t>((c - kHexLeft) / 3);
        color = ByteColor(byte_addr);
      } else if (c >= kAscLeft) {
        const uint32_t byte_addr = addr + static_cast<uint32_t>(c - kAscLeft);
        color = (byte_addr >= kMemSize) ? Qt::red : Qt::black;
      }
      p.setPen(color);
      p.drawText(c * cw, y, QString(text.at(c)));
    }
  }
  p.end();
  out.drawPixmap(0, top, image);

  if (cursor_shown_ && prompt_visible_ && hasFocus()) {
    out.setPen(Qt::black);
    out.setBrush(Qt::NoBrush);
    out.drawRect(QRect(col_ * cw, top + row_ * rh, cw, rh - 1));
  }
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
  const QPoint p = event->position().toPoint();
  if (p.y() < CanvasTop())
    return;
  PlaceCursor((p.y() - CanvasTop()) / RowHeight(), p.x() / ColumnWidth());
}

void MainWindow::TypeKey(char key) {
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
  if (addr >= kMemSize) {
    QApplication::beep();
    return;
  }
  uint8_t* m = memory_.RawData();
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
    m[addr] = low ? static_cast<uint8_t>((m[addr] & 0xF0u) | data)
                  : static_cast<uint8_t>((m[addr] & 0x0Fu) | (data << 4));
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
  update();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
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
    case Qt::Key_F1:
      OnHelp();
      return;
    default:
      break;
  }
  const QString text = event->text();
  if (text.size() == 1 && text.at(0).unicode() < 0x100) {
    TypeKey(static_cast<char>(text.at(0).unicode()));
    return;
  }
  QMainWindow::keyPressEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent* event) {
  ScrollRows(event->angleDelta().y() < 0 ? 1 : -1);
  event->accept();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::FocusIn) {
    ErasePrompt();
  }
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
      case Qt::Key_F1:
        OnHelp();
        return true;
      default:
        break;
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void MainWindow::dropEvent(QDropEvent* event) {
  for (const QUrl& url : event->mimeData()->urls()) {
    const QString path = url.toLocalFile();
    if (!path.isEmpty() && QFileInfo::exists(path)) {
      DropFile(path);
    }
  }
  event->acceptProposedAction();
}

bool MainWindow::LoadSrec(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    Notify(QStringLiteral("error: cannot open file %1").arg(path));
    return false;
  }

  uint32_t low_addr = 0xFFFFFFFFu;
  uint32_t high_addr = 0;
  int line_num = 0;
  int records_read = 0;
  bool end_of_file = false;
  bool srec_error = false;
  QString s0_str;
  QString error_str;
  QString raw_line;

  QTextStream in(&file);
  while (!in.atEnd()) {
    raw_line = in.readLine();
    line_num++;
    if (raw_line.isEmpty())
      continue;

    if (raw_line.size() < 4 || raw_line.at(0) != QLatin1Char('S')) {
      srec_error = true;
      break;
    }
    const QChar s_type = raw_line.at(1);
    uint8_t byte_count = 0;
    if (!ParseHexByte(raw_line, 2, &byte_count)) {
      srec_error = true;
      break;
    }
    records_read++;
    int pos = 4;
    uint32_t loc = 0;

    switch (s_type.unicode()) {
      case '0': {
        if (raw_line.size() < 8) {
          srec_error = true;
          break;
        }
        pos += 4;  // skip 0000 address
        s0_str += QStringLiteral("S0 = ");
        while (pos + 2 < raw_line.size()) {
          uint8_t b = 0;
          if (!ParseHexByte(raw_line, pos, &b)) {
            srec_error = true;
            break;
          }
          pos += 2;
          s0_str += (b >= ' ' && b <= '~') ? QChar(b) : QLatin1Char('.');
        }
        s0_str += QLatin1Char('\n');
        break;
      }
      case '1':
        if (!ParseHexUInt(raw_line, pos, 4, &loc))
          srec_error = true;
        else
          pos += 4;
        break;
      case '2':
        if (!ParseHexUInt(raw_line, pos, 6, &loc))
          srec_error = true;
        else
          pos += 6;
        break;
      case '3':
        if (!ParseHexUInt(raw_line, pos, 8, &loc))
          srec_error = true;
        else
          pos += 8;
        break;
      case '5':
        break;
      case '7':
        if (!ParseHexUInt(raw_line, pos, 8, &loc))
          srec_error = true;
        else {
          start_addr_ = loc;
          end_of_file = true;
        }
        break;
      case '8':
        if (!ParseHexUInt(raw_line, pos, 6, &loc))
          srec_error = true;
        else {
          start_addr_ = loc;
          end_of_file = true;
        }
        break;
      case '9':
        if (!ParseHexUInt(raw_line, pos, 4, &loc))
          srec_error = true;
        else {
          start_addr_ = loc;
          end_of_file = true;
        }
        break;
      default:
        srec_error = true;
        break;
    }

    if (end_of_file || srec_error)
      break;
    if (s_type == QLatin1Char('0') || s_type == QLatin1Char('5'))
      continue;

    while (pos + 2 < raw_line.size()) {
      uint8_t b = 0;
      if (!ParseHexByte(raw_line, pos, &b)) {
        srec_error = true;
        break;
      }
      pos += 2;
      if (loc >= kMemSize) {
        error_str = QStringLiteral("ERROR, Address exceeds $FFFFFF\n");
        srec_error = true;
        break;
      }
      low_addr = std::min(low_addr, loc);
      high_addr = std::max(high_addr, loc);
      memory_.RawData()[loc++] = b;
    }
    if (srec_error)
      break;
  }

  if (records_read == 0) {
    srec_error = true;
  }

  if (srec_error) {
    error_str += QStringLiteral("Invalid data on line %1\n%1: %2\n\nOpen stopped.")
                     .arg(line_num)
                     .arg(raw_line);
    Notify(error_str);
    return false;
  }

  if (low_addr <= high_addr) {
    start_addr_ = low_addr;
    end_addr_ = high_addr;
    length_ = high_addr - low_addr + 1;
  } else {
    start_addr_ = 0;
    end_addr_ = 0;
    length_ = 0;
  }
  Notify(QStringLiteral("S Record loaded successfully.\n\nS0 description record contains:\n") +
         s0_str);
  return true;
}

bool MainWindow::OpenSRecordFile(const QString& path) {
  if (!LoadSrec(path))
    return false;
  setWindowTitle(path);
  start_address_->setText(QStringLiteral("%1").arg(start_addr_, 8, 16, QLatin1Char('0')).toUpper());
  SetTop(start_addr_);
  output_first_address_->setText(
      QStringLiteral("%1").arg(start_addr_ & sim::kAddressMask, 6, 16, QLatin1Char('0')).toUpper());
  s68_length_->setText(
      QStringLiteral("%1").arg(length_ & sim::kAddressMask, 6, 16, QLatin1Char('0')).toUpper());
  output_length_->setText(s68_length_->text());
  s68_end_address_->setText(
      QStringLiteral("%1").arg(end_addr_ & sim::kAddressMask, 6, 16, QLatin1Char('0')).toUpper());
  update();
  return true;
}

int MainWindow::LoadBinary(const QString& path, int split) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    Notify(QStringLiteral("Error openning %1").arg(path));
    return 0;
  }
  uint32_t addr = HexValue(output_first_address_);
  const uint32_t step = (split == 0) ? 1u : static_cast<uint32_t>(split);
  int size = 0;
  char ch = 0;
  while (addr < kMemSize && file.getChar(&ch)) {
    memory_.RawData()[addr] = static_cast<uint8_t>(ch);
    addr += step;
    size += static_cast<int>(step);
  }
  return size;
}

int MainWindow::OpenBinaryFile(const QString& path) {
  const int size = LoadBinary(path, split_);
  if (size > 0) {
    setWindowTitle(path);
    start_address_->setText(QStringLiteral("00000000"));
    SetTop(HexValue(output_first_address_));
    length_ = static_cast<uint32_t>(size);
    output_length_->setText(
        QStringLiteral("%1").arg(length_ & sim::kAddressMask, 6, 16, QLatin1Char('0')).toUpper());
    update();
  }
  return size;
}

bool MainWindow::DropFile(const QString& path) {
  if (OpenSRecordFile(path))
    return true;
  Notify(QStringLiteral("Opening as binary file."));
  return OpenBinaryFile(path) > 0;
}

QString MainWindow::SplitFilePath(const QString& path, int part) {
  const QFileInfo info(path);
  const QString suffix = info.suffix();
  const QString dir_prefix =
      info.path() == QStringLiteral(".") ? QString() : (info.path() + QLatin1Char('/'));
  const QString base = info.completeBaseName();
  if (suffix.isEmpty()) {
    return QStringLiteral("%1%2_%3").arg(dir_prefix, base).arg(part);
  }
  return QStringLiteral("%1%2_%3.%4").arg(dir_prefix, base).arg(part).arg(suffix);
}

bool MainWindow::SaveBinaryTo(const QString& path) {
  const uint32_t from_addr = HexValue(output_first_address_);
  const uint32_t out_length = HexValue(output_length_);
  const uint64_t to_addr = static_cast<uint64_t>(from_addr) + static_cast<uint64_t>(out_length);
  if (out_length == 0 || to_addr > kMemSize) {
    QApplication::beep();
    Notify(QStringLiteral("Invalid memory range."));
    return false;
  }

  const uint8_t* raw = memory_.RawData();
  if (split_ == 0) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
      Notify(QStringLiteral("Error creating %1").arg(path));
      return false;
    }
    if (file.write(reinterpret_cast<const char*>(raw + from_addr), out_length) !=
        static_cast<qint64>(out_length)) {
      Notify(QStringLiteral("Error saving 68K memory to file."));
      return false;
    }
    return true;
  }

  const int num_files = (split_ == 2) ? 2 : 4;
  QFile files[4];
  for (int f = 0; f < num_files; ++f) {
    const QString part_path = SplitFilePath(path, f);
    files[f].setFileName(part_path);
    if (!files[f].open(QIODevice::WriteOnly)) {
      Notify(QStringLiteral("Error creating %1").arg(part_path));
      return false;
    }
  }

  for (uint32_t i = 0; i < out_length; i += static_cast<uint32_t>(num_files)) {
    for (int f = 0; f < num_files; ++f) {
      if (i + static_cast<uint32_t>(f) < out_length) {
        const char b = static_cast<char>(raw[from_addr + i + f]);
        if (files[f].write(&b, 1) != 1) {
          Notify(QStringLiteral("Error saving 68K memory to file."));
          return false;
        }
      }
    }
  }
  return true;
}

bool MainWindow::SaveSRecordTo(const QString& path) {
  constexpr uint32_t kSrecDataBytes = 32;
  const uint32_t from_addr = HexValue(from_);
  const uint32_t to_addr = HexValue(to_);
  if (from_addr > to_addr || to_addr >= kMemSize) {
    QApplication::beep();
    Notify(QStringLiteral("Invalid memory range."));
    return false;
  }

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    Notify(QStringLiteral("Error creating %1").arg(path));
    return false;
  }

  QTextStream out(&file);
  // S0 record ("SRECORD   11CREATED BY EASYBIN").
  out << FinishSRecord(
      QStringLiteral("S00000535245434F5244202020313143524541544544204259204541535942494E"), 0x21);

  const uint32_t out_length = to_addr - from_addr + 1;
  uint32_t srec_addr = from_addr;
  uint32_t data_count = 0;
  const uint8_t* raw = memory_.RawData();

  while (data_count < out_length) {
    QString rec;
    int addr_bytes = 0;
    if ((srec_addr & 0xFFFFu) == srec_addr) {
      rec = QStringLiteral("S1%1").arg(srec_addr, 4, 16, QLatin1Char('0'));
      addr_bytes = 2;
    } else if ((srec_addr & 0xFFFFFFu) == srec_addr) {
      rec = QStringLiteral("S2%1").arg(srec_addr, 6, 16, QLatin1Char('0'));
      addr_bytes = 3;
    } else {
      rec = QStringLiteral("S3%1").arg(srec_addr, 8, 16, QLatin1Char('0'));
      addr_bytes = 4;
    }

    int srec_data = 0;
    while (static_cast<uint32_t>(srec_data) < kSrecDataBytes && data_count < out_length) {
      rec += QStringLiteral("%1").arg(raw[srec_addr++], 2, 16, QLatin1Char('0'));
      srec_data++;
      data_count++;
    }
    out << FinishSRecord(rec.toUpper(), srec_data + addr_bytes + 1);
  }

  const uint32_t exec_start = HexValue(start_address_);
  out << FinishSRecord(QStringLiteral("S7%1").arg(exec_start, 8, 16, QLatin1Char('0')).toUpper(),
                       5);
  return true;
}

void MainWindow::NewEnvironment(bool confirm) {
  if (confirm) {
    const auto res = QMessageBox::question(
        this, QStringLiteral("Caution"),
        QStringLiteral("The current environment will be lost, click OK to continue."),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (res != QMessageBox::Ok)
      return;
  }
  memory_.Erase();
  setWindowTitle(QString::fromLatin1(kTitle));
  start_address_->setText(QStringLiteral("00000000"));
  s68_end_address_->setText(QStringLiteral("000000"));
  s68_length_->setText(QStringLiteral("000000"));
  address_->setText(QStringLiteral("000000"));
  output_first_address_->setText(QStringLiteral("000000"));
  output_length_->setText(QStringLiteral("000000"));
  length_ = 0;
  mem_addr_ = 0;
  start_addr_ = 0;
  end_addr_ = 0;
  SetSplitIndex(0);
  update();
}

void MainWindow::CopyRange() {
  const uint32_t from_addr = HexValue(from_);
  const uint32_t to_addr = HexValue(to_);
  const uint32_t bytes = HexValue(bytes_);
  if (to_addr == from_addr || bytes == 0 || static_cast<uint64_t>(from_addr) + bytes > kMemSize ||
      static_cast<uint64_t>(to_addr) + bytes > kMemSize) {
    Notify(QStringLiteral("Invalid copy range"));
    return;
  }
  std::memmove(memory_.RawData() + to_addr, memory_.RawData() + from_addr, bytes);
  update();
}

void MainWindow::FillRange() {
  const uint32_t from_addr = HexValue(from_);
  const uint32_t to_addr = HexValue(to_);
  const uint8_t fill_data = static_cast<uint8_t>(HexValue(fill_byte_) & 0xFFu);
  if (from_addr > to_addr || to_addr >= kMemSize) {
    Notify(QStringLiteral("Invalid memory range"));
    return;
  }
  std::memset(memory_.RawData() + from_addr, fill_data, to_addr - from_addr + 1);
  update();
}

void MainWindow::OnOpenSRecord() {
  const QString path = QFileDialog::getOpenFileName(
      this, QStringLiteral("Open S-Record"), QString(),
      QStringLiteral("EASy68K S-Record (*.S68 *.s68);;Motorola S-Record (*.S3 "
                     "*.S19 *.s3 *.s19);;Other S-Record (*.*)"));
  if (!path.isEmpty()) {
    OpenSRecordFile(path);
  }
}

void MainWindow::OnOpenBinary() {
  const QString path =
      QFileDialog::getOpenFileName(this, QStringLiteral("Open Binary"), QString(),
                                   QStringLiteral("Binary File (*.BIN *.bin);;Any File (*.*)"));
  if (!path.isEmpty()) {
    OpenBinaryFile(path);
  }
}

void MainWindow::OnSaveSRecord() {
  const QString msg = QStringLiteral(
                          "S-Record file information:\n\nFrom:$%1  To:$%2\nStart "
                          "Address:$%3\n\nCreate S-Record?")
                          .arg(from_->text(), to_->text(), start_address_->text());
  if (QMessageBox::question(this, QStringLiteral("S-Record Creation"), msg,
                            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
    return;
  }
  QFileDialog dialog(this, QStringLiteral("Save S-Record"));
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setNameFilter(
      QStringLiteral("EASy68K S-Record (*.S68);;Motorola S-Record (*.S3);;Other "
                     "S-Record (*.*)"));
  dialog.setDefaultSuffix(QStringLiteral("S68"));
  if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty()) {
    SaveSRecordTo(dialog.selectedFiles().first());
  }
}

void MainWindow::OnSaveBinary() {
  QFileDialog dialog(this, QStringLiteral("Save Binary"));
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setNameFilter(QStringLiteral("Binary File (*.BIN);;Other Binary File (*.*)"));
  dialog.setDefaultSuffix(QStringLiteral("BIN"));
  if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty()) {
    SaveBinaryTo(dialog.selectedFiles().first());
  }
}

void MainWindow::OnAbout() {
  QMessageBox::about(
      this, QStringLiteral("About EASyBIN"),
      QStringLiteral("<b>%1</b><br>By: Chuck Kelly<br><br>"
                     "The author of this software assumes no liability for its "
                     "functionality and is not responsible for damages, losses or "
                     "liabilities as a result of using this software. Use at your own "
                     "risk.<br><br>"
                     "EASyBIN is distributed under the GNU general public use license. "
                     "See Help for more information.<br><br>"
                     "www.easy68k.com<br>"
                     "Use the EASy68K forum to report undocumented features.")
          .arg(QString::fromLatin1(kTitle)));
}

void MainWindow::OnHelp() {
  Notify(QStringLiteral("EASyBIN Help (Task 13.25)"));
}

}  // namespace easym68k::easybin
