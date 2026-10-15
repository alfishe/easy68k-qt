// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// SimIoScreen: the Sim68K I/O window contents (simIOu.cpp TsimIO), shared
// by the text (QtTextIO), graphics (QtGraphicsIO) and keyboard back ends and
// shown by the I/O window widget.
//
// Text and graphics draw on one canvas, as in the original: a back buffer
// (BackBuffer) plus the displayed image, which is the back buffer itself
// unless double buffering is on (Trap #15 task 92 mode 17), when it only
// changes on Repaint (task 94). The onscreen characters are kept in a text
// grid for task 22.
//
// Threading: the simulator thread calls the output and blocking input
// functions; the GUI thread feeds keys and takes snapshots. Every function
// is thread-safe (one mutex). Changed() is emitted, coalesced, whenever the
// displayed image changes; the widget calls Snapshot(), which re-arms it.
// ReadLine()/ReadChar() block the simulator thread until the input is
// complete or CancelInput() is called.

#ifndef EASY68K_GUI_SIMULATOR_SIM_IO_SCREEN_H_
#define EASY68K_GUI_SIMULATOR_SIM_IO_SCREEN_H_

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <QFont>
#include <QImage>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QRgb>
#include <QSize>
#include <string>
#include <string_view>
#include <vector>

class QPainter;

namespace easym68k::gui {

class SimIoScreen : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMinWidth = 640;  // simIOu.cpp MIN_WIDTH / MIN_HEIGHT
  static constexpr int kMinHeight = 480;
  static constexpr int kMaxRows = 128;  // MAX_ROWS / MAX_COLS
  static constexpr int kMaxCols = 256;
  static constexpr int kMaxInput = 79;  // keys accepted before input ends

  // Pen modes (Trap #15 task 92): the VCL TPenMode order (Win32 ROP2).
  enum PenMode {
    kPenBlack = 0,
    kPenWhite,
    kPenNop,
    kPenNot,
    kPenCopy,
    kPenNotCopy,
    kPenMergePenNot,
    kPenMaskPenNot,
    kPenMergeNotPen,
    kPenMaskNotPen,
    kPenMerge,
    kPenNotMerge,
    kPenMask,
    kPenNotMask,
    kPenXor,
    kPenNotXor,
  };
  static constexpr int kDoubleBufferOff = 16;
  static constexpr int kDoubleBufferOn = 17;

  explicit SimIoScreen(QObject* parent = nullptr);
  ~SimIoScreen() override;

  // ---- Settings (GUI) -------------------------------------------------------
  // The output font (FontDialogSimIO): applied now and by ResetForRun();
  // task 21 starts from it. `color` is a TColor (0x00BBGGRR).
  void SetBaseFont(const QFont& font, uint32_t color);
  void SetTextWrap(bool wrap);  // Options: wrap text at the window edge

  // Program start (STARTSIM.CPP): clears the window, keeps its size, resets
  // pen mode/width/colours, double buffering, font, and the input state.
  void ResetForRun();

  // ---- Window ---------------------------------------------------------------
  // Task 33: 0,0 keeps the size; values are raised to 640x480. Clears.
  void SetWindowSize(int width, int height);
  void GetWindowSize(int* width, int* height) const;

  // ---- Text output (tasks 0, 1, 6, 11, 13-15, 17, 20-22, 25) ------------------
  void CharOut(char ch);
  void TextOut(std::string_view text);
  void TextOutCRLF(std::string_view text);
  void ClearScreen();
  void GotoRC(int row, int col);
  void GetRC(int* col, int* row) const;
  char CharAt(int row, int col) const;
  QSize CellSize() const;  // column width and row height in pixels
  void ScrollRect(int row, int col, int height, int width, int direction);
  void SetFontProperties(uint32_t color, uint32_t style);

  // ---- Input (simulator thread; block until done or cancelled) --------------
  bool ReadLine(std::string* line);  // tasks 2, 4, 18: false if cancelled
  bool ReadChar(char* ch);           // task 5
  bool IsKeyPending() const;         // task 7
  void SetKeyboardEcho(bool enable);
  void SetInputPrompt(bool enable);
  void SetInputLFDisplay(bool enable);
  // Stop/reset while the program waits for input: wakes and fails ReadLine()
  // / ReadChar() until ResumeInput() (or ResetForRun()).
  void CancelInput();
  void ResumeInput();
  bool InputActive() const;

  // ---- Keyboard (GUI thread) ------------------------------------------------
  void KeyPress(char key);                   // a typed character (FormKeyPress)
  void KeyDown(uint8_t vk);                  // Windows VK code (FormKeyDown)
  void KeyUp(uint8_t vk);                    // (FormKeyUp)
  void ClearKeys();                          // window activated (FormActivate)
  void GetKeyState(long* codes) const;       // task 19
  void SetKeyCommandsEnabled(bool enabled);  // task 24
  bool KeyCommandsEnabled() const;
  // Called (outside the lock) on every key down (true) / up (false): the
  // keyboard IRQ of task 62.
  void SetKeyEventHook(std::function<void(bool down)> hook);
  // `hook` runs on the simulator thread while it waits for input, each time
  // Poke() is called (the session serves GUI requests meanwhile).
  void SetWaitHook(std::function<void()> hook) { wait_hook_ = std::move(hook); }
  // Called (under the screen lock) for every character the output log
  // records (simIOu.cpp charOut(), doCRLF(), echo of typed input).
  void SetOutputLogHook(std::function<void(char)> hook) { output_log_ = std::move(hook); }
  void Poke();

  // ---- Graphics (tasks 80-96) -------------------------------------------------
  void SetPenColor(uint32_t color);
  void SetFillColor(uint32_t color);
  void DrawPixel(int x, int y);
  uint32_t GetPixel(int x, int y) const;  // TColor, 0xFFFFFFFF outside
  void Line(int x1, int y1, int x2, int y2);
  void LineTo(int x, int y);
  void MoveTo(int x, int y);
  void GetXY(int* x, int* y) const;
  void Rectangle(int x1, int y1, int x2, int y2);
  void Ellipse(int x1, int y1, int x2, int y2);
  void FloodFill(int x, int y);
  void UnfilledRectangle(int x1, int y1, int x2, int y2);
  void UnfilledEllipse(int x1, int y1, int x2, int y2);
  void SetDrawingMode(int mode);
  void SetPenWidth(int width);
  void Repaint();
  void DrawText(std::string_view text, int x, int y);

  // ---- Display (GUI thread) -------------------------------------------------
  QImage Snapshot();  // the displayed image; re-arms Changed()
  // The input prompt (the original flashes '_' at the cursor during input,
  // drawn on the window only, not in the back buffer).
  struct Prompt {
    bool active = false;
    QRect cell;  // cursor cell in canvas pixels
    QFont font;
    int ascent = 0;
    QRgb text_color = 0;
    QRgb background = 0;
  };
  Prompt PromptInfo() const;
  void SetWindowVisible(bool visible);  // the widget's visibility

 signals:
  void Changed();        // the displayed image changed
  void ShowRequested();  // output/input while hidden
  void WindowSizeChanged(int width, int height);
  void Bell();                            // BEL output (task 6, '\a')
  void KeyCommandsChanged(bool enabled);  // task 24 (any thread)

 private:
  // All *Locked functions require mu_.
  void SetWindowSizeLocked(int width, int height);
  void SetTextSizeLocked();
  void ApplyFontLocked(const QFont& font, uint32_t color);
  void CharOutLocked(char ch);
  void DrawGlyphsLocked(const QString& text, int x, int y);
  void DoCRLFLocked();
  void ClearLocked();
  void GotoRCLocked(int row, int col);
  void ScrollLocked();
  void FillBlackLocked(const QRect& rect);
  void CopyRectLocked(const QRect& dst, const QRect& src);
  void KeyPressLocked(char key);
  void EchoInputCharLocked(char key);
  void FinishInputLocked();
  // Draws a shape with the pen (outline) and brush (fill) in the current
  // pen mode; `paint` issues the QPainter calls, `bounds` covers the shape.
  void DrawShapeLocked(const QRect& bounds, bool filled,
                       const std::function<void(QPainter&)>& paint);
  static QRect Normalized(int x1, int y1, int x2, int y2);

  void MarkDirty(bool dirty);  // emits Changed() once until Snapshot()
  void WaitForInput(std::unique_lock<std::mutex>& lock);
  void RequestShow(bool request);

  mutable std::mutex mu_;
  std::condition_variable input_cv_;

  // Canvas
  QImage back_;   // BackBuffer
  QImage front_;  // the displayed image while double buffering
  bool double_buffer_ = false;
  int can_width_ = kMinWidth;
  int can_height_ = kMinHeight;

  // Text
  QFont base_font_;
  uint32_t base_color_ = 0x00FFFFFF;  // clWhite
  QFont font_;
  uint32_t font_color_ = 0x00FFFFFF;
  int row_height_ = 1;
  int col_width_ = 1;
  int ascent_ = 0;
  int text_rows_ = 0;
  int text_cols_ = 0;
  std::vector<char> text_;  // kMaxRows x kMaxCols onscreen characters
  int row_ = 0;
  int col_ = 0;
  int text_x_ = 0;
  int text_y_ = 0;
  bool text_wrap_ = false;

  // Graphics
  uint32_t pen_color_ = 0x00FFFFFF;
  uint32_t brush_color_ = 0;
  int pen_width_ = 1;
  int pen_mode_ = kPenCopy;
  QPoint pen_pos_;

  // Input
  bool input_mode_ = false;
  bool char_input_ = false;
  bool input_done_ = false;
  bool cancel_input_ = false;
  bool poked_ = false;
  std::function<void()> wait_hook_;
  std::function<void(char)> output_log_;
  std::string key_buf_;
  std::string line_;
  char input_char_ = 0;
  char pending_key_ = 0;
  bool keyboard_echo_ = true;
  bool input_prompt_ = true;
  bool input_lf_display_ = true;

  // Keyboard state
  std::array<bool, 256> keys_{};
  uint8_t key_down_code_ = 0;
  uint8_t key_up_code_ = 0;
  bool key_commands_enabled_ = true;
  std::function<void(bool)> key_hook_;

  // Notification
  std::atomic<bool> update_pending_{false};
  std::atomic<bool> visible_{false};
  std::atomic<bool> show_pending_{false};
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_SIM_IO_SCREEN_H_
