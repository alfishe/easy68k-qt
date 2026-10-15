// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Port of the Sim68K I/O window (simIOu.cpp TsimIO): text output, keyboard
// input and graphics. Behaviour follows the original except for the fixes
// listed in PROGRESS.md (13.1 decisions); "simIOu.cpp" comments name the
// original function.

#include "sim_io_screen.h"

#include <algorithm>
#include <cstring>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <utility>

namespace easym68k::gui {

namespace {

// TColor (0x00BBGGRR) <-> QRgb.
QRgb ToRgb(uint32_t c) {
  return qRgb(c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF);
}
uint32_t ToTColor(QRgb c) {
  return static_cast<uint32_t>(qRed(c)) | (static_cast<uint32_t>(qGreen(c)) << 8) |
         (static_cast<uint32_t>(qBlue(c)) << 16);
}

// Win32 ROP2 on 24-bit colours (VCL TPenMode order).
uint32_t Rop2(int mode, uint32_t p, uint32_t d) {
  uint32_t r = d;
  switch (mode) {
    case SimIoScreen::kPenBlack:
      r = 0;
      break;
    case SimIoScreen::kPenWhite:
      r = 0xFFFFFF;
      break;
    case SimIoScreen::kPenNop:
      r = d;
      break;
    case SimIoScreen::kPenNot:
      r = ~d;
      break;
    case SimIoScreen::kPenCopy:
      r = p;
      break;
    case SimIoScreen::kPenNotCopy:
      r = ~p;
      break;
    case SimIoScreen::kPenMergePenNot:
      r = p | ~d;
      break;
    case SimIoScreen::kPenMaskPenNot:
      r = p & ~d;
      break;
    case SimIoScreen::kPenMergeNotPen:
      r = ~p | d;
      break;
    case SimIoScreen::kPenMaskNotPen:
      r = ~p & d;
      break;
    case SimIoScreen::kPenMerge:
      r = p | d;
      break;
    case SimIoScreen::kPenNotMerge:
      r = ~(p | d);
      break;
    case SimIoScreen::kPenMask:
      r = p & d;
      break;
    case SimIoScreen::kPenNotMask:
      r = ~(p & d);
      break;
    case SimIoScreen::kPenXor:
      r = p ^ d;
      break;
    case SimIoScreen::kPenNotXor:
      r = ~(p ^ d);
      break;
    default:
      break;
  }
  return r & 0xFFFFFF;
}

QString Latin1(char ch) {
  return QString(QChar(static_cast<unsigned char>(ch)));
}

}  // namespace

SimIoScreen::SimIoScreen(QObject* parent)
    : QObject(parent), text_(static_cast<size_t>(kMaxRows) * kMaxCols, ' ') {
  // FontDialogSimIO default: Fixedsys 9, white. Fixedsys only exists on
  // Windows; the hints select a fixed-pitch font elsewhere.
  base_font_ = QFont(QStringLiteral("Fixedsys"));
  base_font_.setPointSize(9);
  base_font_.setStyleHint(QFont::TypeWriter);
  base_font_.setFixedPitch(true);
  std::lock_guard<std::mutex> lock(mu_);
  font_ = base_font_;
  SetWindowSizeLocked(kMinWidth, kMinHeight);
}

SimIoScreen::~SimIoScreen() {
  CancelInput();
}

// ---------------------------------------------------------------------------
// Settings and window
// ---------------------------------------------------------------------------

void SimIoScreen::SetBaseFont(const QFont& font, uint32_t color) {
  {
    std::lock_guard<std::mutex> lock(mu_);
    base_font_ = font;
    base_color_ = color;
    ApplyFontLocked(font, color);
  }
}

void SimIoScreen::SetTextWrap(bool wrap) {
  std::lock_guard<std::mutex> lock(mu_);
  text_wrap_ = wrap;
}

void SimIoScreen::ResetForRun() {
  int w = 0, h = 0;
  bool key_commands_restored = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    ClearLocked();
    SetWindowSizeLocked(0, 0);
    pen_mode_ = kPenCopy;
    double_buffer_ = false;
    front_ = QImage();
    pen_width_ = 1;
    pen_color_ = 0x00FFFFFF;  // clWhite
    brush_color_ = 0;         // clBlack
    // The original assigns the font without recomputing the text size;
    // ApplyFontLocked() does both.
    ApplyFontLocked(base_font_, base_color_);
    input_mode_ = false;
    char_input_ = false;
    input_done_ = false;
    cancel_input_ = false;
    pending_key_ = 0;
    key_buf_.clear();
    keyboard_echo_ = true;
    input_prompt_ = true;
    input_lf_display_ = true;
    key_commands_restored = !key_commands_enabled_;
    key_commands_enabled_ = true;  // Form1->restoreMenuTask19()
    w = can_width_;
    h = can_height_;
  }
  if (key_commands_restored)
    emit KeyCommandsChanged(true);
  emit WindowSizeChanged(w, h);
  MarkDirty(true);
}

void SimIoScreen::SetWindowSize(int width, int height) {
  int w = 0, h = 0;
  {
    std::lock_guard<std::mutex> lock(mu_);
    SetWindowSizeLocked(width, height);
    w = can_width_;
    h = can_height_;
  }
  emit WindowSizeChanged(w, h);
  MarkDirty(true);
}

void SimIoScreen::GetWindowSize(int* width, int* height) const {
  std::lock_guard<std::mutex> lock(mu_);
  *width = can_width_;
  *height = can_height_;
}

// simIOu.cpp setWindowSize(): 0,0 keeps the size; at least 640x480; the
// canvas is cleared. The text grid is cleared with it (the original kept the
// characters of the cleared canvas for task 22).
void SimIoScreen::SetWindowSizeLocked(int width, int height) {
  if (width == 0 && height == 0) {
    width = can_width_;
    height = can_height_;
  }
  can_height_ = std::max(height, static_cast<int>(kMinHeight));
  can_width_ = std::max(width, static_cast<int>(kMinWidth));
  back_ = QImage(can_width_, can_height_, QImage::Format_RGB32);
  back_.setDotsPerMeterX(3780);  // 96 dpi, as the original (PixelsPerInch)
  back_.setDotsPerMeterY(3780);
  back_.fill(Qt::black);
  if (double_buffer_)
    front_ = back_;
  std::fill(text_.begin(), text_.end(), ' ');
  SetTextSizeLocked();
}

// simIOu.cpp setTextSize(): row height of "Xp", column width of "W".
void SimIoScreen::SetTextSizeLocked() {
  QFontMetrics fm(font_, &back_);
  row_height_ = std::max(1, fm.height());
  col_width_ = std::max(1, fm.horizontalAdvance(QLatin1Char('W')));
  ascent_ = fm.ascent();
  text_rows_ = std::min(can_height_ / row_height_, static_cast<int>(kMaxRows));
  text_cols_ = std::min(can_width_ / col_width_, static_cast<int>(kMaxCols));
}

void SimIoScreen::ApplyFontLocked(const QFont& font, uint32_t color) {
  font_ = font;
  font_color_ = color;
  SetTextSizeLocked();
  GotoRCLocked(row_, col_);
}

// ---------------------------------------------------------------------------
// Text output
// ---------------------------------------------------------------------------

void SimIoScreen::CharOut(char ch) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    CharOutLocked(ch);
    dirty = !double_buffer_;
  }
  RequestShow(true);
  if (ch == '\a')
    emit Bell();
  MarkDirty(dirty);
}

void SimIoScreen::TextOut(std::string_view text) {
  bool dirty = false;
  bool bell = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    for (char ch : text) {
      CharOutLocked(ch);
      bell |= ch == '\a';
    }
    dirty = !double_buffer_;
  }
  RequestShow(true);
  if (bell)
    emit Bell();
  MarkDirty(dirty);
}

void SimIoScreen::TextOutCRLF(std::string_view text) {
  bool dirty = false;
  bool bell = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    for (char ch : text) {
      CharOutLocked(ch);
      bell |= ch == '\a';
    }
    DoCRLFLocked();
    dirty = !double_buffer_;
  }
  RequestShow(true);
  if (bell)
    emit Bell();
  MarkDirty(dirty);
}

// simIOu.cpp charOut(): control characters move the cursor; others are drawn
// at the cursor and stored in the text grid. BEL is signalled by the caller.
void SimIoScreen::CharOutLocked(char ch) {
  switch (ch) {
    case '\a':
      break;
    case '\b':
      col_--;
      break;
    case '\f':
      ClearLocked();  // like HOME
      break;
    case '\n':
      row_++;
      break;
    case '\r':
      col_ = 0;
      break;
    case '\t':
      col_ += 5;
      break;
    case '\v':
      row_ += 4;
      break;
    default:
      if (static_cast<unsigned char>(ch) >= 0x20) {
        DrawGlyphsLocked(Latin1(ch), text_x_, text_y_);
        text_[static_cast<size_t>(row_) * kMaxCols + col_] = ch;
        col_++;
      }
      break;
  }
  GotoRCLocked(row_, col_);
}

// VCL TCanvas.TextOut: opaque background in the fill colour; the pen moves
// to the end of the text.
void SimIoScreen::DrawGlyphsLocked(const QString& text, int x, int y) {
  QPainter p(&back_);
  p.setFont(font_);
  const int w = p.fontMetrics().horizontalAdvance(text);
  p.fillRect(QRect(x, y, w, row_height_), QColor(ToRgb(brush_color_)));
  p.setPen(QColor(ToRgb(font_color_)));
  p.drawText(x, y + ascent_, text);
  pen_pos_ = QPoint(x + w, y);
}

void SimIoScreen::DoCRLFLocked() {
  col_ = 0;
  row_++;
  GotoRCLocked(row_, col_);
}

void SimIoScreen::ClearScreen() {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    ClearLocked();
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

// simIOu.cpp clear(): home, black canvas, blank text grid.
void SimIoScreen::ClearLocked() {
  GotoRCLocked(0, 0);
  back_.fill(Qt::black);
  std::fill(text_.begin(), text_.end(), ' ');
}

void SimIoScreen::GotoRC(int row, int col) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    GotoRCLocked(row, col);
    dirty = !double_buffer_;  // may have scrolled
  }
  MarkDirty(dirty);
}

// simIOu.cpp gotorc(): a row below the window scrolls once and goes to the
// last row; a column past the edge wraps only with the text-wrap option.
void SimIoScreen::GotoRCLocked(int row, int col) {
  if (row < 0) {
    row = 0;
  } else if (row_height_ * row > can_height_ - row_height_) {
    ScrollLocked();
    row = can_height_ / row_height_ - 1;
  }
  if (col < 0) {
    col = 0;
  } else if (col_width_ * col > can_width_ - col_width_ && text_wrap_) {
    DoCRLFLocked();
    return;
  }
  text_y_ = row_height_ * row;
  row_ = std::min(row, kMaxRows - 1);
  text_x_ = col_width_ * col;
  col_ = std::min(col, kMaxCols - 1);
}

void SimIoScreen::GetRC(int* col, int* row) const {
  std::lock_guard<std::mutex> lock(mu_);
  *col = col_;
  *row = row_;
}

// simIOu.cpp getCharAt(): row and column are limited to the grid size.
char SimIoScreen::CharAt(int row, int col) const {
  std::lock_guard<std::mutex> lock(mu_);
  row = std::clamp(row, 0, kMaxRows - 1);
  col = std::clamp(col, 0, kMaxCols - 1);
  return text_[static_cast<size_t>(row) * kMaxCols + col];
}

QSize SimIoScreen::CellSize() const {
  std::lock_guard<std::mutex> lock(mu_);
  return QSize(col_width_, row_height_);
}

void SimIoScreen::FillBlackLocked(const QRect& rect) {
  QPainter p(&back_);
  p.fillRect(rect, Qt::black);
}

// BitBlt-style copy of a canvas rectangle (source and destination may
// overlap).
void SimIoScreen::CopyRectLocked(const QRect& dst, const QRect& src) {
  if (src.isEmpty())
    return;
  const QImage piece = back_.copy(src);
  QPainter p(&back_);
  p.setCompositionMode(QPainter::CompositionMode_Source);
  p.drawImage(dst.topLeft(), piece);
}

// simIOu.cpp scroll(): the whole window up one row.
void SimIoScreen::ScrollLocked() {
  CopyRectLocked(QRect(0, 0, can_width_, can_height_ - row_height_),
                 QRect(0, row_height_, can_width_, can_height_ - row_height_));
  FillBlackLocked(QRect(0, can_height_ - row_height_, can_width_, row_height_));
  if (text_rows_ < 1)
    return;
  for (int i = 0; i < text_rows_ - 1; i++)
    std::memcpy(&text_[static_cast<size_t>(i) * kMaxCols],
                &text_[static_cast<size_t>(i + 1) * kMaxCols], text_cols_);
  std::memset(&text_[static_cast<size_t>(text_rows_ - 1) * kMaxCols], ' ', text_cols_);
}

// simIOu.cpp scrollRect(): scroll a text rectangle one character up (0),
// down (1), left (2) or right (3); invalid rectangles are ignored.
void SimIoScreen::ScrollRect(int r, int c, int h, int w, int dir) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (r + h >= kMaxRows || c + w >= kMaxCols || w <= 0 || h <= 0 || r < 0 || c < 0)
      return;
    const int cw = col_width_;
    const int rh = row_height_;
    auto rect = [](int left, int top, int right, int bottom) {
      return QRect(left, top, right - left, bottom - top);
    };
    auto at = [this](int i, int j) -> char& {
      return text_[static_cast<size_t>(i) * kMaxCols + j];
    };
    const QRect top_rect = rect(c * cw, r * rh, (c + w) * cw, (r + h - 1) * rh);
    const QRect bot_rect = rect(c * cw, (r + 1) * rh, (c + w) * cw, (r + h) * rh);
    const QRect left_rect = rect(c * cw, r * rh, (c + w - 1) * cw, (r + h) * rh);
    const QRect right_rect = rect((c + 1) * cw, r * rh, (c + w) * cw, (r + h) * rh);
    switch (dir) {
      case 0:  // up
        CopyRectLocked(top_rect, bot_rect);
        FillBlackLocked(rect(c * cw, (r + h - 1) * rh, (c + w) * cw, (r + h) * rh));
        for (int i = r; i < r + h - 1; i++)
          for (int j = c; j < c + w; j++)
            at(i, j) = at(i + 1, j);
        for (int j = c; j < c + w; j++)
          at(r + h - 1, j) = ' ';
        break;
      case 1:  // down
        CopyRectLocked(bot_rect, top_rect);
        FillBlackLocked(rect(c * cw, r * rh, (c + w) * cw, (r + 1) * rh));
        for (int i = r + h - 1; i > r; i--)
          for (int j = c; j < c + w; j++)
            at(i, j) = at(i - 1, j);
        for (int j = c; j < c + w; j++)
          at(r, j) = ' ';
        break;
      case 2:  // left
        CopyRectLocked(left_rect, right_rect);
        FillBlackLocked(rect((c + w - 1) * cw, r * rh, (c + w) * cw, (r + h) * rh));
        for (int i = r; i < r + h; i++) {
          for (int j = c; j < c + w - 1; j++)
            at(i, j) = at(i, j + 1);
          at(i, c + w - 1) = ' ';
        }
        break;
      case 3:  // right
        CopyRectLocked(right_rect, left_rect);
        FillBlackLocked(rect(c * cw, r * rh, (c + 1) * cw, (r + h) * rh));
        for (int i = r; i < r + h; i++) {
          for (int j = c + w - 1; j > c; j--)
            at(i, j) = at(i, j - 1);
          at(i, c) = ' ';
        }
        break;
      default:
        return;  // reserved
    }
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

// simIOu.cpp setFontProperties(): colour, style bits (bold, italic,
// underline, strike-out; combined here, the original kept only the last),
// and in the high word font number and point size.
void SimIoScreen::SetFontProperties(uint32_t color, uint32_t style) {
  std::lock_guard<std::mutex> lock(mu_);
  QFont f = base_font_;
  f.setBold((style & 0x01) != 0);
  f.setItalic((style & 0x02) != 0);
  f.setUnderline((style & 0x04) != 0);
  f.setStrikeOut((style & 0x08) != 0);
  const uint32_t name_size = style >> 16;
  if (name_size != 0) {
    static const char* const kNames[] = {nullptr,       "Fixedsys",       "Courier",
                                         "Courier New", "Lucida Console", "Lucida Sans Typewriter",
                                         "Consolas",    "Terminal"};
    const uint32_t name = (name_size >> 8) & 0xFF;
    if (name >= 1 && name <= 7)
      f.setFamily(QString::fromLatin1(kNames[name]));
    if ((name_size & 0xFF) != 0)
      f.setPointSize(static_cast<int>(name_size & 0xFF));
  }
  f.setStyleHint(QFont::TypeWriter);
  f.setFixedPitch(true);
  ApplyFontLocked(f, color);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

// simIOu.cpp textIn() + FormKeyPress(): at most 79 keys; Enter (or a key past
// the limit) ends the input with CR LF.
bool SimIoScreen::ReadLine(std::string* line) {
  std::unique_lock<std::mutex> lock(mu_);
  if (cancel_input_)
    return false;
  input_mode_ = true;
  char_input_ = false;
  input_done_ = false;
  key_buf_.clear();
  if (pending_key_ != 0) {
    const char key = pending_key_;
    pending_key_ = 0;
    KeyPressLocked(key);
  }
  lock.unlock();
  RequestShow(true);
  MarkDirty(true);  // prompt
  lock.lock();
  input_cv_.wait(lock, [this] { return input_done_ || cancel_input_; });
  if (!input_done_) {
    input_mode_ = false;
    return false;
  }
  input_done_ = false;
  *line = line_;
  return true;
}

// simIOu.cpp charIn(): a key typed before the request is returned at once.
bool SimIoScreen::ReadChar(char* ch) {
  std::unique_lock<std::mutex> lock(mu_);
  if (pending_key_ != 0) {
    *ch = pending_key_;
    pending_key_ = 0;
    EchoInputCharLocked(*ch);
    const bool dirty = !double_buffer_;
    lock.unlock();
    MarkDirty(dirty);
    return true;
  }
  if (cancel_input_)
    return false;
  input_mode_ = true;
  char_input_ = true;
  input_done_ = false;
  lock.unlock();
  RequestShow(true);
  MarkDirty(true);
  lock.lock();
  input_cv_.wait(lock, [this] { return input_done_ || cancel_input_; });
  if (!input_done_) {
    input_mode_ = false;
    char_input_ = false;
    return false;
  }
  input_done_ = false;
  *ch = input_char_;
  return true;
}

bool SimIoScreen::IsKeyPending() const {
  std::lock_guard<std::mutex> lock(mu_);
  return pending_key_ != 0;
}

void SimIoScreen::SetKeyboardEcho(bool enable) {
  std::lock_guard<std::mutex> lock(mu_);
  keyboard_echo_ = enable;
}

void SimIoScreen::SetInputPrompt(bool enable) {
  std::lock_guard<std::mutex> lock(mu_);
  input_prompt_ = enable;
}

void SimIoScreen::SetInputLFDisplay(bool enable) {
  std::lock_guard<std::mutex> lock(mu_);
  input_lf_display_ = enable;
}

void SimIoScreen::CancelInput() {
  {
    std::lock_guard<std::mutex> lock(mu_);
    cancel_input_ = true;
    input_mode_ = false;
    char_input_ = false;
  }
  input_cv_.notify_all();
  MarkDirty(true);  // prompt gone
}

void SimIoScreen::ResumeInput() {
  std::lock_guard<std::mutex> lock(mu_);
  cancel_input_ = false;
}

bool SimIoScreen::InputActive() const {
  std::lock_guard<std::mutex> lock(mu_);
  return input_mode_;
}

void SimIoScreen::KeyPress(char key) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    KeyPressLocked(key);
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

// Echo of a character read by task 5 (FormKeyPress, charInput).
void SimIoScreen::EchoInputCharLocked(char key) {
  if (!keyboard_echo_)
    return;
  CharOutLocked(key);
  if (key == '\r' && input_lf_display_)
    CharOutLocked('\n');
}

// simIOu.cpp FormKeyPress(). Echoed characters are stored in the text grid
// like other output, and Backspace keeps editing at the 79-key limit (the
// original drew them only, and ended the input on Backspace at the limit).
void SimIoScreen::KeyPressLocked(char key) {
  if (!input_mode_) {
    pending_key_ = key;
    return;
  }
  if (char_input_) {
    input_char_ = key;
    char_input_ = false;
    EchoInputCharLocked(key);
    FinishInputLocked();
    return;
  }
  if (key == '\b') {
    if (key_buf_.empty())
      return;
    key_buf_.pop_back();
    GotoRCLocked(row_, col_ - 1);
    if (keyboard_echo_) {
      DrawGlyphsLocked(QStringLiteral(" "), text_x_, text_y_);
      text_[static_cast<size_t>(row_) * kMaxCols + col_] = ' ';
    }
    return;
  }
  if (key == '\r' || static_cast<int>(key_buf_.size()) >= kMaxInput) {
    DoCRLFLocked();
    line_ = key_buf_;
    FinishInputLocked();
    return;
  }
  key_buf_.push_back(key);
  if (keyboard_echo_) {
    DrawGlyphsLocked(Latin1(key), text_x_, text_y_);
    text_[static_cast<size_t>(row_) * kMaxCols + col_] = key;
  }
  GotoRCLocked(row_, col_ + 1);
}

void SimIoScreen::FinishInputLocked() {
  input_mode_ = false;
  input_done_ = true;
  input_cv_.notify_all();
}

// ---------------------------------------------------------------------------
// Keyboard state
// ---------------------------------------------------------------------------

void SimIoScreen::KeyDown(uint8_t vk) {
  std::function<void(bool)> hook;
  {
    std::lock_guard<std::mutex> lock(mu_);
    keys_[vk] = true;
    key_down_code_ = vk;
    hook = key_hook_;
  }
  if (hook)
    hook(true);
}

void SimIoScreen::KeyUp(uint8_t vk) {
  std::function<void(bool)> hook;
  {
    std::lock_guard<std::mutex> lock(mu_);
    keys_[vk] = false;
    key_up_code_ = vk;
    hook = key_hook_;
  }
  if (hook)
    hook(false);
}

void SimIoScreen::ClearKeys() {
  std::lock_guard<std::mutex> lock(mu_);
  key_down_code_ = 0;
  key_up_code_ = 0;
  keys_.fill(false);
}

// simIOu.cpp getKeyState(): 0 returns the last key up/down codes, else
// $FF per byte for each of the four requested keys that is down.
void SimIoScreen::GetKeyState(long* codes) const {
  std::lock_guard<std::mutex> lock(mu_);
  const auto code = static_cast<uint32_t>(*codes);
  if (code == 0) {
    *codes = static_cast<long>((static_cast<uint32_t>(key_up_code_) << 16) | key_down_code_);
    return;
  }
  uint32_t state = 0;
  for (int i = 0; i < 4; i++)
    if (keys_[(code >> (8 * i)) & 0xFF])
      state |= 0xFFu << (8 * i);
  *codes = static_cast<long>(static_cast<int32_t>(state));
}

void SimIoScreen::SetKeyCommandsEnabled(bool enabled) {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    changed = key_commands_enabled_ != enabled;
    key_commands_enabled_ = enabled;
  }
  if (changed)
    emit KeyCommandsChanged(enabled);
}

bool SimIoScreen::KeyCommandsEnabled() const {
  std::lock_guard<std::mutex> lock(mu_);
  return key_commands_enabled_;
}

void SimIoScreen::SetKeyEventHook(std::function<void(bool)> hook) {
  std::lock_guard<std::mutex> lock(mu_);
  key_hook_ = std::move(hook);
}

// ---------------------------------------------------------------------------
// Graphics
// ---------------------------------------------------------------------------

void SimIoScreen::SetPenColor(uint32_t color) {
  std::lock_guard<std::mutex> lock(mu_);
  pen_color_ = color & 0xFFFFFF;
}

void SimIoScreen::SetFillColor(uint32_t color) {
  std::lock_guard<std::mutex> lock(mu_);
  brush_color_ = color & 0xFFFFFF;
}

// Canvas->Pixels[x][y] = pen colour (SetPixel: the pen mode does not apply).
void SimIoScreen::DrawPixel(int x, int y) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (!back_.rect().contains(x, y))
      return;
    back_.setPixel(x, y, ToRgb(pen_color_));
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

uint32_t SimIoScreen::GetPixel(int x, int y) const {
  std::lock_guard<std::mutex> lock(mu_);
  if (!back_.rect().contains(x, y))
    return 0xFFFFFFFFu;  // CLR_INVALID
  return ToTColor(back_.pixel(x, y));
}

QRect SimIoScreen::Normalized(int x1, int y1, int x2, int y2) {
  return QRect(QPoint(std::min(x1, x2), std::min(y1, y2)),
               QPoint(std::max(x1, x2), std::max(y1, y2)));
}

// Every pixel is drawn once, with the pen (outline) or the brush (inside),
// combined with the canvas by the pen mode as GDI does. Copy mode paints
// directly; the other modes rasterise a label mask first.
void SimIoScreen::DrawShapeLocked(const QRect& bounds, bool filled,
                                  const std::function<void(QPainter&)>& paint) {
  if (pen_mode_ == kPenNop)
    return;
  auto make_pen = [this](QRgb color) {
    QPen pen{QColor(color)};
    if (pen_width_ > 1) {
      pen.setWidth(pen_width_);
      pen.setCapStyle(Qt::RoundCap);  // GDI geometric pen
      pen.setJoinStyle(Qt::RoundJoin);
    } else {
      pen.setWidth(0);  // cosmetic: one pixel, both end points drawn
    }
    return pen;
  };
  if (pen_mode_ == kPenCopy) {
    QPainter p(&back_);
    p.setPen(make_pen(ToRgb(pen_color_)));
    p.setBrush(filled ? QBrush(QColor(ToRgb(brush_color_))) : QBrush(Qt::NoBrush));
    paint(p);
    return;
  }
  const int margin = pen_width_ + 2;
  const QRect area = bounds.adjusted(-margin, -margin, margin, margin).intersected(back_.rect());
  if (area.isEmpty())
    return;
  constexpr QRgb kBrushLabel = 1;
  constexpr QRgb kPenLabel = 2;
  QImage mask(area.size(), QImage::Format_RGB32);
  mask.fill(0);
  {
    QPainter p(&mask);
    p.translate(-area.topLeft());
    p.setPen(make_pen(kPenLabel));
    p.setBrush(filled ? QBrush(QColor(kBrushLabel)) : QBrush(Qt::NoBrush));
    paint(p);
  }
  for (int y = 0; y < area.height(); y++) {
    const auto* m = reinterpret_cast<const QRgb*>(mask.constScanLine(y));
    auto* d = reinterpret_cast<QRgb*>(back_.scanLine(area.top() + y)) + area.left();
    for (int x = 0; x < area.width(); x++) {
      const QRgb label = m[x] & 0xFF;
      if (label == 0)
        continue;
      const uint32_t src = label == kPenLabel ? pen_color_ : brush_color_;
      const uint32_t dst = ToTColor(d[x]);
      d[x] = ToRgb(Rop2(pen_mode_, src, dst));
    }
  }
}

// simIOu.cpp line(): the end point is drawn in the pen mode too (the
// original plotted it afterwards with SetPixel, ignoring the mode).
void SimIoScreen::Line(int x1, int y1, int x2, int y2) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    DrawShapeLocked(Normalized(x1, y1, x2, y2), false,
                    [&](QPainter& p) { p.drawLine(x1, y1, x2, y2); });
    pen_pos_ = QPoint(x2, y2);
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

void SimIoScreen::LineTo(int x, int y) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    const QPoint from = pen_pos_;
    DrawShapeLocked(Normalized(from.x(), from.y(), x, y), false,
                    [&](QPainter& p) { p.drawLine(from, QPoint(x, y)); });
    pen_pos_ = QPoint(x, y);
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

void SimIoScreen::MoveTo(int x, int y) {
  std::lock_guard<std::mutex> lock(mu_);
  pen_pos_ = QPoint(x, y);
}

// One pen position for both canvases (the original read the window's while
// drawing moved the back buffer's when double buffering).
void SimIoScreen::GetXY(int* x, int* y) const {
  std::lock_guard<std::mutex> lock(mu_);
  *x = pen_pos_.x();
  *y = pen_pos_.y();
}

// Rectangle/ellipse cover x1..x2, y1..y2 inclusive (the original's +1).
void SimIoScreen::Rectangle(int x1, int y1, int x2, int y2) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    const QRect r = Normalized(x1, y1, x2, y2);
    DrawShapeLocked(r, true, [&](QPainter& p) {
      p.drawRect(QRectF(r.left(), r.top(), r.width() - 1, r.height() - 1));
    });
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

void SimIoScreen::UnfilledRectangle(int x1, int y1, int x2, int y2) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    const QRect r = Normalized(x1, y1, x2, y2);
    DrawShapeLocked(r, false, [&](QPainter& p) {
      p.drawRect(QRectF(r.left(), r.top(), r.width() - 1, r.height() - 1));
    });
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

// The original skipped ellipses with x1+y1+x2+y2 >= 32768 (a Windows 95
// limit); here every ellipse is drawn.
void SimIoScreen::Ellipse(int x1, int y1, int x2, int y2) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    const QRect r = Normalized(x1, y1, x2, y2);
    DrawShapeLocked(r, true, [&](QPainter& p) {
      p.drawEllipse(QRectF(r.left(), r.top(), r.width() - 1, r.height() - 1));
    });
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

void SimIoScreen::UnfilledEllipse(int x1, int y1, int x2, int y2) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    const QRect r = Normalized(x1, y1, x2, y2);
    DrawShapeLocked(r, false, [&](QPainter& p) {
      p.drawEllipse(QRectF(r.left(), r.top(), r.width() - 1, r.height() - 1));
    });
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

// simIOu.cpp floodFill() (FloodFill fsSurface): the 4-connected area of the
// colour at x,y is filled with the fill colour. Scanline fill.
void SimIoScreen::FloodFill(int x, int y) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (!back_.rect().contains(x, y))
      return;
    const QRgb target = back_.pixel(x, y);
    const QRgb fill = ToRgb(brush_color_);
    if (target == fill)
      return;
    const int w = back_.width();
    const int h = back_.height();
    std::vector<QPoint> stack{QPoint(x, y)};
    while (!stack.empty()) {
      QPoint seed = stack.back();
      stack.pop_back();
      auto* line = reinterpret_cast<QRgb*>(back_.scanLine(seed.y()));
      if (line[seed.x()] != target)
        continue;
      int left = seed.x();
      while (left > 0 && line[left - 1] == target)
        left--;
      int right = seed.x();
      while (right < w - 1 && line[right + 1] == target)
        right++;
      for (int i = left; i <= right; i++)
        line[i] = fill;
      for (int ny : {seed.y() - 1, seed.y() + 1}) {
        if (ny < 0 || ny >= h)
          continue;
        const auto* next = reinterpret_cast<const QRgb*>(back_.constScanLine(ny));
        bool in_run = false;
        for (int i = left; i <= right; i++) {
          const bool match = next[i] == target;
          if (match && !in_run)
            stack.emplace_back(i, ny);
          in_run = match;
        }
      }
    }
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

// simIOu.cpp setDrawingMode(): 16/17 double buffering off/on, 0-15 pen
// mode; other values are ignored (the original passed them to the VCL).
void SimIoScreen::SetDrawingMode(int mode) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    mode &= 0xFF;
    if (mode == kDoubleBufferOff) {
      double_buffer_ = false;
      front_ = QImage();
      dirty = true;
    } else if (mode == kDoubleBufferOn) {
      if (!double_buffer_)
        front_ = back_;
      double_buffer_ = true;
    } else if (mode <= kPenNotXor) {
      pen_mode_ = mode;
    }
  }
  MarkDirty(dirty);
}

void SimIoScreen::SetPenWidth(int width) {
  std::lock_guard<std::mutex> lock(mu_);
  pen_width_ = width & 0xFF;
}

// Task 94 (FormPaint): show the back buffer.
void SimIoScreen::Repaint() {
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (double_buffer_)
      front_ = back_;
  }
  MarkDirty(true);
}

// simIOu.cpp drawText(): graphics text at a pixel position, not stored in
// the text grid.
void SimIoScreen::DrawText(std::string_view text, int x, int y) {
  bool dirty = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    DrawGlyphsLocked(QString::fromLatin1(text.data(), static_cast<qsizetype>(text.size())), x, y);
    dirty = !double_buffer_;
  }
  MarkDirty(dirty);
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

QImage SimIoScreen::Snapshot() {
  std::lock_guard<std::mutex> lock(mu_);
  update_pending_ = false;
  return double_buffer_ ? front_ : back_;
}

SimIoScreen::Prompt SimIoScreen::PromptInfo() const {
  std::lock_guard<std::mutex> lock(mu_);
  Prompt prompt;
  if (!input_mode_ || !input_prompt_)
    return prompt;
  prompt.active = true;
  prompt.cell = QRect(col_width_ * col_, text_y_, col_width_, row_height_);
  prompt.font = font_;
  prompt.ascent = ascent_;
  prompt.text_color = ToRgb(font_color_);
  prompt.background = ToRgb(brush_color_);
  return prompt;
}

void SimIoScreen::SetWindowVisible(bool visible) {
  visible_ = visible;
  show_pending_ = false;
}

void SimIoScreen::MarkDirty(bool dirty) {
  if (dirty && !update_pending_.exchange(true))
    emit Changed();
}

void SimIoScreen::RequestShow(bool request) {
  if (request && !visible_ && !show_pending_.exchange(true))
    emit ShowRequested();
}

}  // namespace easym68k::gui
