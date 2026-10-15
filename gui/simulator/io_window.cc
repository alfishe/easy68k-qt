// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "io_window.h"

#include <algorithm>
#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

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
  canvas_width_ = w;
  canvas_height_ = h;
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

// setWindowSize(): the canvas size; in full screen the canvas is scaled.
void IoWindow::Resize(int width, int height) {
  canvas_width_ = width;
  canvas_height_ = height;
  if (shown_full_)
    update();
  else
    setFixedSize(width, height);
}

void IoWindow::SetFullScreen(bool full) {
  full_screen_ = full;
  ApplyWindowMode();
}

// setupWindow(): full screen is a borderless window on top covering the
// chosen screen; window mode restores the position and the canvas size.
void IoWindow::ApplyWindowMode() {
  if (!isVisible() || full_screen_ == shown_full_)
    return;
  shown_full_ = full_screen_;
  if (full_screen_) {
    saved_pos_ = pos();
    const QList<QScreen*> screens = QGuiApplication::screens();
    if (monitor_ < 0 || monitor_ >= screens.size())
      monitor_ = 0;
    QScreen* screen = screens.isEmpty() ? nullptr : screens.at(monitor_);
    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    if (screen != nullptr)
      setGeometry(screen->geometry());
    showFullScreen();
    raise();
    activateWindow();
  } else {
    setWindowFlags(Qt::Window);
    showNormal();
    setFixedSize(canvas_width_, canvas_height_);
    move(saved_pos_);
  }
}

QRect IoWindow::CanvasRect() const {
  if (!shown_full_ || canvas_width_ <= 0 || canvas_height_ <= 0)
    return QRect(0, 0, canvas_width_, canvas_height_);
  const double scale = std::min(static_cast<double>(width()) / canvas_width_,
                                static_cast<double>(height()) / canvas_height_);
  const int w = static_cast<int>(canvas_width_ * scale);
  const int h = static_cast<int>(canvas_height_ * scale);
  return QRect((width() - w) / 2, (height() - h) / 2, w, h);
}

QPoint IoWindow::CanvasToWindow(QPoint p) const {
  const QRect r = CanvasRect();
  if (canvas_width_ <= 0 || canvas_height_ <= 0)
    return p;
  return QPoint(r.left() + p.x() * r.width() / canvas_width_,
                r.top() + p.y() * r.height() / canvas_height_);
}

QPoint IoWindow::WindowToCanvas(QPoint p) const {
  const QRect r = CanvasRect();
  if (r.width() <= 0 || r.height() <= 0)
    return p;
  return QPoint((p.x() - r.left()) * canvas_width_ / r.width(),
                (p.y() - r.top()) * canvas_height_ / r.height());
}

void IoWindow::paintEvent(QPaintEvent*) {
  QPainter p(this);
  if (shown_full_) {
    p.fillRect(rect(), Qt::black);
    const QRect r = CanvasRect();
    p.translate(r.topLeft());
    if (canvas_width_ > 0 && canvas_height_ > 0)
      p.scale(static_cast<double>(r.width()) / canvas_width_,
              static_cast<double>(r.height()) / canvas_height_);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
  }
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
// Alt+key gives no character (WM_SYSKEYDOWN). Unless task 24 disabled them:
// Ctrl+Tab goes to the memory window, Alt+Enter toggles full screen,
// Shift+Insert types up to 255 characters of the clipboard.
void IoWindow::keyPressEvent(QKeyEvent* event) {
  const uint8_t vk = QtKeyToVirtualKey(event->key(), event->modifiers() & Qt::KeypadModifier);
  if (vk != 0)
    screen_->KeyDown(vk);  // also for auto-repeat, as Windows repeats WM_KEYDOWN
  event->accept();
  const Qt::KeyboardModifiers mods = event->modifiers();
  if (screen_->KeyCommandsEnabled()) {
    const int key = event->key();
    if (key == Qt::Key_Tab && (mods & Qt::ControlModifier)) {
      emit NextWindowRequested();
      return;
    }
    if ((key == Qt::Key_Return || key == Qt::Key_Enter) && (mods & Qt::AltModifier)) {
      if (!event->isAutoRepeat())
        SetFullScreen(!full_screen_);
      return;
    }
    if (key == Qt::Key_Insert && (mods & Qt::ShiftModifier)) {
      const QByteArray text = QGuiApplication::clipboard()->text().toLatin1().left(255);
      for (const char c : text)
        screen_->KeyPress(c);
      return;
    }
  }
  if ((mods & Qt::AltModifier) && !(mods & Qt::ControlModifier))
    return;
  const QString text = event->text();
  if (text.size() == 1 && text.at(0).unicode() < 0x100)
    screen_->KeyPress(static_cast<char>(text.at(0).unicode()));
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
  const QPoint p = WindowToCanvas(event->position().toPoint());
  peripheral_->MouseDown(p.x(), p.y(), ButtonsOf(event, false));
}

void IoWindow::mouseDoubleClickEvent(QMouseEvent* event) {
  const QPoint p = WindowToCanvas(event->position().toPoint());
  peripheral_->MouseDown(p.x(), p.y(), ButtonsOf(event, true));
}

void IoWindow::mouseReleaseEvent(QMouseEvent* event) {
  const QPoint p = WindowToCanvas(event->position().toPoint());
  peripheral_->MouseUp(p.x(), p.y(), ButtonsOf(event, false));
}

void IoWindow::mouseMoveEvent(QMouseEvent* event) {
  const QPoint p = WindowToCanvas(event->position().toPoint());
  const auto m = event->modifiers();
  peripheral_->MouseMove(p.x(), p.y(), m & Qt::ShiftModifier, m & Qt::AltModifier,
                         m & Qt::ControlModifier);
}

void IoWindow::showEvent(QShowEvent* event) {
  screen_->SetWindowVisible(true);
  QWidget::showEvent(event);
  if (full_screen_ != shown_full_)
    QMetaObject::invokeMethod(this, &IoWindow::ApplyWindowMode, Qt::QueuedConnection);
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
