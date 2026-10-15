// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "io_window.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include "key_map.h"
#include "qt_peripheral_io.h"
#include "sim_io_screen.h"

namespace easym68k::gui {

namespace {

QtPeripheralIO::Buttons ButtonsOf(const QMouseEvent* e, bool double_click) {
  QtPeripheralIO::Buttons b;
  b.left = (e->buttons() & Qt::LeftButton) != 0;
  b.right = (e->buttons() & Qt::RightButton) != 0;
  b.middle = (e->buttons() & Qt::MiddleButton) != 0;
  b.double_click = double_click;
  b.shift = (e->modifiers() & Qt::ShiftModifier) != 0;
  b.alt = (e->modifiers() & Qt::AltModifier) != 0;
  b.ctrl = (e->modifiers() & Qt::ControlModifier) != 0;
  return b;
}

}  // namespace

IoWindow::IoWindow(SimIoScreen* screen, QtPeripheralIO* peripheral, QWidget* parent)
    : QWidget(parent, Qt::Window), screen_(screen), peripheral_(peripheral) {
  setWindowTitle(QStringLiteral("Sim68K I/O"));
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  setAttribute(Qt::WA_OpaquePaintEvent);
  int w = 0, h = 0;
  screen_->GetWindowSize(&w, &h);
  Resize(w, h);
  connect(screen_, &SimIoScreen::Changed, this, [this] { update(); });
  connect(screen_, &SimIoScreen::ShowRequested, this, [this] {
    show();
    raise();
  });
  connect(screen_, &SimIoScreen::Bell, this, [] { QApplication::beep(); });
  connect(screen_, &SimIoScreen::WindowSizeChanged, this, &IoWindow::Resize);
  prompt_timer_.setInterval(500);
  connect(&prompt_timer_, &QTimer::timeout, this, [this] {
    prompt_visible_ = !prompt_visible_;
    update();
  });
  prompt_timer_.start();
}

void IoWindow::Resize(int width, int height) {
  setFixedSize(width, height);
}

void IoWindow::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.drawImage(0, 0, screen_->Snapshot());
  // simIOu.cpp promptTimer(): '_' at the cursor, flashing, on the window only.
  const SimIoScreen::Prompt prompt = screen_->PromptInfo();
  if (prompt.active && prompt_visible_) {
    p.fillRect(prompt.cell, QColor(prompt.background));
    p.setFont(prompt.font);
    p.setPen(QColor(prompt.text_color));
    p.drawText(prompt.cell.left(), prompt.cell.top() + prompt.ascent, QStringLiteral("_"));
  }
}

// simIOu.cpp FormKeyDown()/FormKeyPress(): the VK code for the key state
// and key IRQ, the character for input.
void IoWindow::keyPressEvent(QKeyEvent* event) {
  const uint8_t vk = QtKeyToVirtualKey(event->key(), event->modifiers() & Qt::KeypadModifier);
  if (vk != 0)
    screen_->KeyDown(vk);  // also for auto-repeat, as Windows repeats WM_KEYDOWN
  const QString text = event->text();
  if (text.size() == 1 && text.at(0).unicode() < 0x100)
    screen_->KeyPress(static_cast<char>(text.at(0).unicode()));
  event->accept();
}

void IoWindow::keyReleaseEvent(QKeyEvent* event) {
  if (event->isAutoRepeat())
    return;
  const uint8_t vk = QtKeyToVirtualKey(event->key(), event->modifiers() & Qt::KeypadModifier);
  if (vk != 0)
    screen_->KeyUp(vk);
  event->accept();
}

void IoWindow::mousePressEvent(QMouseEvent* event) {
  const QPoint p = event->position().toPoint();
  peripheral_->MouseDown(p.x(), p.y(), ButtonsOf(event, false));
}

void IoWindow::mouseDoubleClickEvent(QMouseEvent* event) {
  const QPoint p = event->position().toPoint();
  peripheral_->MouseDown(p.x(), p.y(), ButtonsOf(event, true));
}

void IoWindow::mouseReleaseEvent(QMouseEvent* event) {
  const QPoint p = event->position().toPoint();
  peripheral_->MouseUp(p.x(), p.y(), ButtonsOf(event, false));
}

void IoWindow::mouseMoveEvent(QMouseEvent* event) {
  const QPoint p = event->position().toPoint();
  const auto m = event->modifiers();
  peripheral_->MouseMove(p.x(), p.y(), m & Qt::ShiftModifier, m & Qt::AltModifier,
                         m & Qt::ControlModifier);
}

void IoWindow::showEvent(QShowEvent* event) {
  screen_->SetWindowVisible(true);
  QWidget::showEvent(event);
}

void IoWindow::hideEvent(QHideEvent* event) {
  screen_->SetWindowVisible(false);
  QWidget::hideEvent(event);
}

// simIOu.cpp FormActivate(): the key state is cleared.
void IoWindow::changeEvent(QEvent* event) {
  if (event->type() == QEvent::ActivationChange && isActiveWindow())
    screen_->ClearKeys();
  QWidget::changeEvent(event);
}

bool IoWindow::focusNextPrevChild(bool) {
  return false;
}

}  // namespace easym68k::gui
