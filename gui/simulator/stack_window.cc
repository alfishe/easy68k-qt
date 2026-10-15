// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "stack_window.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "easym68k/sim/types.h"

namespace easym68k::gui {

namespace {

constexpr int kPanelWidth = 60;
const QColor kAqua(0, 255, 255);
const QColor kYellow(255, 255, 0);

QFont StackFont() {  // Stack1.dfm: Courier New, Height -13
  QFont font(QStringLiteral("Courier New"));
  font.setPixelSize(13);
  font.setStyleHint(QFont::TypeWriter);
  font.setFixedPitch(true);
  return font;
}

}  // namespace

StackWindow::StackWindow(const sim::Memory* memory, std::function<bool()> readable, QWidget* parent)
    : QWidget(parent, Qt::Window), memory_(memory), readable_(std::move(readable)) {
  setWindowTitle(QStringLiteral("68000 Stack"));
  setFixedWidth(250);
  setMinimumHeight(240);
  resize(250, 400);
  setFont(StackFont());
  setAutoFillBackground(true);
  QPalette pal = palette();
  pal.setColor(QPalette::Window, Qt::white);
  setPalette(pal);
  setFocusPolicy(Qt::StrongFocus);

  panel_ = new QWidget(this);
  panel_->setFixedWidth(kPanelWidth);
  panel_->setAutoFillBackground(true);
  auto* column = new QVBoxLayout(panel_);
  column->setContentsMargins(4, 8, 4, 8);
  auto* view = new QLabel(QStringLiteral("View"), panel_);
  QFont small = view->font();
  small.setPixelSize(11);
  view->setFont(small);
  column->addWidget(view);
  auto* up = new QToolButton(panel_);
  up->setArrowType(Qt::UpArrow);
  up->setAutoRepeat(true);
  up->setToolTip(QStringLiteral("Lower addresses"));
  connect(up, &QToolButton::clicked, this, [this] { ScrollRows(-1); });
  auto* down = new QToolButton(panel_);
  down->setArrowType(Qt::DownArrow);
  down->setAutoRepeat(true);
  down->setToolTip(QStringLiteral("Higher addresses"));
  connect(down, &QToolButton::clicked, this, [this] { ScrollRows(1); });
  column->addWidget(up);
  column->addWidget(down);
  which_ = new QComboBox(panel_);
  which_->addItems({QStringLiteral("A0"), QStringLiteral("A1"), QStringLiteral("A2"),
                    QStringLiteral("A3"), QStringLiteral("A4"), QStringLiteral("A5"),
                    QStringLiteral("A6"), QStringLiteral("US"), QStringLiteral("SS")});
  which_->setCurrentIndex(8);  // SS
  connect(which_, &QComboBox::currentIndexChanged, this, [this] { UpdateState(state_); });
  column->addWidget(which_);
  column->addStretch();
}

uint32_t StackWindow::SelectedRegister() const {
  const int i = which_->currentIndex();
  if (i == 7)
    return state_.a[7];  // US
  if (i == 8)
    return state_.ssp;  // SS
  return state_.a[i];
}

// Stack1.cpp updateDisplay(). The register's byte is compared as a 24-bit
// address (the original's unmasked value never matched above $FFFFFF).
void StackWindow::UpdateState(const sim::CpuState& state) {
  state_ = state;
  const uint32_t value = SelectedRegister();
  areg_addr_ = value & sim::kAddressMask;
  mid_addr_ = value - value % 2;
  update();
}

void StackWindow::ScrollRows(int rows) {
  if (readable_ && !readable_())
    return;
  mid_addr_ += static_cast<uint32_t>(rows * 4);
  update();
}

int StackWindow::Rows() const {
  const int h = QFontMetrics(font()).height();
  return h > 0 ? height() / h : 0;
}

uint32_t StackWindow::TopAddress() const {
  return (mid_addr_ - static_cast<uint32_t>((Rows() / 2) * 4)) & sim::kAddressMask;
}

QString StackWindow::RowText(int row) const {
  const uint32_t addr = (TopAddress() + static_cast<uint32_t>(row) * 4) & sim::kAddressMask;
  QString text = QStringLiteral("%1: ").arg(addr, 8, 16, QLatin1Char('0')).toUpper();
  for (uint32_t i = 0; i < 4; i++) {
    const uint8_t b = memory_->RawData()[(addr + i) & sim::kAddressMask];
    text += QStringLiteral("%1 ").arg(b, 2, 16, QLatin1Char('0')).toUpper();
  }
  return text;
}

QColor StackWindow::Background(int row, int byte) const {
  const uint32_t addr =
      (TopAddress() + static_cast<uint32_t>(row) * 4 + static_cast<uint32_t>(byte)) &
      sim::kAddressMask;
  if (addr == areg_addr_)
    return kAqua;
  if (addr == (state_.GetSP() & sim::kAddressMask))
    return kYellow;
  return Qt::white;
}

QColor StackWindow::Foreground(int row, int byte) const {
  const uint32_t addr =
      (TopAddress() + static_cast<uint32_t>(row) * 4 + static_cast<uint32_t>(byte)) &
      sim::kAddressMask;
  return (memory_->GetFlags(addr) & sim::kMemInvalid) ? QColor(Qt::red) : QColor(Qt::black);
}

// Stack1.cpp DisplayStack().
void StackWindow::paintEvent(QPaintEvent*) {
  QPainter out(this);
  if (readable_ && !readable_()) {
    out.drawPixmap(0, 0, last_image_);
    return;
  }
  QPixmap image(width() - kPanelWidth, height());
  image.fill(Qt::white);
  {
    QPainter p(&image);
    p.setFont(font());
    const QFontMetrics fm(font());
    const int rh = fm.height();
    for (int r = 0; r < Rows(); r++) {
      const QString text = RowText(r);
      const int y = r * rh;
      p.setPen(Qt::black);
      const QString address = text.left(10);
      p.drawText(0, y + fm.ascent(), address);
      int x = fm.horizontalAdvance(address);
      for (int i = 0; i < 4; i++) {
        const QString cell = text.mid(10 + i * 3, 3);
        const int w = fm.horizontalAdvance(cell);
        p.fillRect(QRect(x, y, w, rh), Background(r, i));
        p.setPen(Foreground(r, i));
        p.drawText(x, y + fm.ascent(), cell);
        x += w;
      }
    }
  }
  last_image_ = image;
  out.drawPixmap(0, 0, image);
}

void StackWindow::wheelEvent(QWheelEvent* event) {
  ScrollRows(event->angleDelta().y() < 0 ? 1 : -1);  // wheel down: higher addresses
  event->accept();
}

void StackWindow::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Tab && (event->modifiers() & Qt::ControlModifier)) {
    emit NextWindowRequested();
    return;
  }
  QWidget::keyPressEvent(event);
}

void StackWindow::resizeEvent(QResizeEvent* event) {
  panel_->setGeometry(width() - kPanelWidth, 0, kPanelWidth, height());
  QWidget::resizeEvent(event);
}

}  // namespace easym68k::gui
