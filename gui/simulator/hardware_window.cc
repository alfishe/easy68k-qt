// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "hardware_window.h"

#include <QApplication>
#include <QGroupBox>
#include <QKeyEvent>
#include <QPainter>
#include <QPixmap>
#include <QRegularExpressionValidator>
#include <QStyleOptionButton>

#include "easym68k/sim/types.h"
#include "qt_simulator_env.h"
#include "sim_session.h"

namespace easym68k::gui {

namespace {

constexpr uint32_t kMemSize = sim::kMemorySize;
const QColor kRed(255, 0, 0);          // clRed
const QColor kDarkMaroon(0x40, 0, 0);  // clDrkMaroon (0x02000040)
const QColor kMaroon(128, 0, 0);       // clMaroon
const QColor kGray(128, 128, 128);     // clGray
const QColor kSilver(192, 192, 192);   // clSilver

// A TBitBtn / TSpeedButton with a glyph from hardwareu.dfm. Push buttons
// react to any key (pb0KeyDown()/pb0KeyUp()), switches show the left half
// of their glyph when off and the right half when on (blGlyphLeft /
// blGlyphRight), speed buttons their second frame when disabled.
class GlyphButton : public QAbstractButton {
 public:
  enum Kind { kSwitch, kPush, kSpeed };

  GlyphButton(Kind kind, const QString& glyph, QWidget* parent)
      : QAbstractButton(parent), kind_(kind), glyph_(glyph) {
    setCursor(kind == kSpeed ? Qt::ArrowCursor : Qt::PointingHandCursor);
    setCheckable(kind == kSwitch);
    setFocusPolicy(kind == kSpeed ? Qt::NoFocus : Qt::StrongFocus);
  }

 protected:
  void paintEvent(QPaintEvent* /*event*/) override {
    QPainter p(this);
    QStyleOptionButton option;
    option.initFrom(this);
    option.state |= isDown() ? QStyle::State_Sunken : QStyle::State_Raised;
    style()->drawPrimitive(QStyle::PE_PanelButtonCommand, &option, &p, this);
    int frames = 1, frame = 0;
    if (kind_ == kSwitch) {
      frames = 2;
      frame = isChecked() ? 1 : 0;
    } else if (kind_ == kSpeed) {
      frames = 2;
      frame = isEnabled() ? 0 : 1;
    }
    const int w = glyph_.width() / frames;
    const int shift = isDown() ? 1 : 0;
    const QPoint at((width() - w) / 2 + shift, (height() - glyph_.height()) / 2 + shift);
    p.drawPixmap(at, glyph_, QRect(frame * w, 0, w, glyph_.height()));
  }

  void keyPressEvent(QKeyEvent* event) override {
    if (kind_ != kPush || !IsButtonKey(event))
      return QAbstractButton::keyPressEvent(event);
    if (!event->isAutoRepeat()) {
      setDown(true);
      emit pressed();
    }
  }

  void keyReleaseEvent(QKeyEvent* event) override {
    if (kind_ != kPush || !IsButtonKey(event))
      return QAbstractButton::keyReleaseEvent(event);
    if (!event->isAutoRepeat() && isDown()) {
      setDown(false);
      emit released();
    }
  }

 private:
  // Tab moves the focus (the original's key up then went to the next control
  // and the button stayed pressed); shortcuts pass.
  static bool IsButtonKey(const QKeyEvent* event) {
    const int key = event->key();
    if (key == Qt::Key_Tab || key == Qt::Key_Backtab || key == Qt::Key_Shift ||
        key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Meta)
      return false;
    return (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier)) == 0;
  }

  Kind kind_;
  QPixmap glyph_;
};

QLabel* Label(const QString& text, QWidget* parent, const QRect& geometry) {
  auto* label = new QLabel(text, parent);
  label->setGeometry(geometry);
  return label;
}

// The grey "00" in front of every address field (MaskEdit2 ...).
void HighByte(QWidget* parent, int x, int y) {
  auto* e = new QLineEdit(QStringLiteral("00"), parent);
  e->setGeometry(x, y, 21, 22);
  e->setReadOnly(true);
  e->setFocusPolicy(Qt::NoFocus);
  e->setTextMargins(0, 0, 0, 0);
  QPalette pal = e->palette();
  pal.setColor(QPalette::Base, kSilver);
  pal.setColor(QPalette::Text, Qt::black);
  e->setPalette(pal);
}

// TMaskEdit 'aaaaaa' with addrKeyPress(): six hex digits, upper case; other
// keys beep. (The original also refused Backspace.)
QLineEdit* AddressEdit(QWidget* parent, int x, int y) {
  HighByte(parent, x, y);
  auto* e = new QLineEdit(parent);
  e->setGeometry(x + 19, y, 54, 22);
  e->setMaxLength(6);
  e->setValidator(
      new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[0-9A-Fa-f]{0,6}")), e));
  QPalette pal = e->palette();
  pal.setColor(QPalette::Text, Qt::black);
  e->setPalette(pal);
  QObject::connect(e, &QLineEdit::inputRejected, e, [] { QApplication::beep(); });
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
  return e->text().toUInt(nullptr, 16);  // empty = 0
}

void SetBackground(QWidget* w, const QColor& background, const QColor& text) {
  w->setAutoFillBackground(true);
  QPalette pal = w->palette();
  pal.setColor(QPalette::Window, background);
  pal.setColor(QPalette::WindowText, text);
  w->setPalette(pal);
}

QGroupBox* Group(const QString& title, const QColor& color, QWidget* parent,
                 const QRect& geometry) {
  auto* box = new QGroupBox(title, parent);
  box->setGeometry(geometry);
  SetBackground(box, color, Qt::white);
  QFont font = box->font();
  font.setPixelSize(13);
  box->setFont(font);
  return box;
}

void DrawBevel(QPainter* p, const QRect& r) {  // TPanel bvRaised
  p->setPen(Qt::white);
  p->drawLine(r.topLeft(), r.topRight());
  p->drawLine(r.topLeft(), r.bottomLeft());
  p->setPen(kGray.darker(150));
  p->drawLine(r.bottomLeft(), r.bottomRight());
  p->drawLine(r.topRight(), r.bottomRight());
}

}  // namespace

// Panel2: eight digits of TShape segments (a-g and the point h).
class HardwareWindow::SegmentPanel : public QWidget {
 public:
  explicit SegmentPanel(QWidget* parent) : QWidget(parent) { bytes_.fill(0xFF); }
  void SetByte(int digit, uint8_t value) { bytes_[digit] = value; }
  uint8_t Byte(int digit) const { return bytes_[digit]; }

 protected:
  void paintEvent(QPaintEvent* /*event*/) override {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    DrawBevel(&p, rect().adjusted(0, 0, -1, -1));
    struct Seg {
      int x, y, w, h;
    };
    static constexpr Seg kSegs[8] = {
        {4, 8, 20, 5},    // a
        {24, 12, 5, 20},  // b
        {24, 36, 5, 20},  // c
        {4, 56, 20, 5},   // d
        {0, 36, 5, 20},   // e
        {0, 12, 5, 20},   // f
        {4, 32, 20, 5},   // g
        {32, 56, 5, 5},   // h
    };
    p.setPen(Qt::black);
    for (int digit = 0; digit < 8; digit++) {
      const int x0 = 8 + (7 - digit) * 40;
      for (int s = 0; s < 8; s++) {
        const QRect r(x0 + kSegs[s].x, kSegs[s].y, kSegs[s].w, kSegs[s].h);
        p.setBrush((bytes_[digit] >> s) & 1 ? kRed : kDarkMaroon);
        p.drawRect(r.adjusted(0, 0, -1, -1));
      }
    }
  }

 private:
  std::array<uint8_t, 8> bytes_{};
};

// Panel1: eight round LEDs.
class HardwareWindow::LedPanel : public QWidget {
 public:
  explicit LedPanel(QWidget* parent) : QWidget(parent) {}
  void SetByte(uint8_t value) { byte_ = value; }
  uint8_t Byte() const { return byte_; }

 protected:
  void paintEvent(QPaintEvent* /*event*/) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), kGray);
    DrawBevel(&p, rect().adjusted(0, 0, -1, -1));
    p.setPen(QPen(kMaroon, 2));
    for (int bit = 0; bit < 8; bit++) {
      p.setBrush((byte_ >> bit) & 1 ? kRed : kDarkMaroon);
      p.drawEllipse(QRect(16 + (7 - bit) * 40 + 1, 8 + 1, 15, 15));
    }
  }

 private:
  uint8_t byte_ = 0xFF;  // the form's initial red
};

HardwareWindow::HardwareWindow(SimSession* session, QWidget* parent)
    : QWidget(parent, Qt::Window), session_(session) {
  setWindowTitle(QStringLiteral("EASy68K Hardware"));
  setFixedSize(422, 495);
  QFont font = this->font();
  font.setPixelSize(11);
  setFont(font);

  BuildDisplays();
  BuildAddresses();
  BuildInterrupts();
  BuildMemoryMap();

  connect(session_, &SimSession::MemorySnapshot, this, &HardwareWindow::OnSnapshot);
  connect(session_, &SimSession::MemoryMapChanged, this, &HardwareWindow::ShowMemoryMap);
  connect(session_, &SimSession::HardwareEnabledChanged, this, [this](bool enabled) {
    for (QAbstractButton* b : irq_)
      b->setEnabled(enabled);
    reset_->setEnabled(enabled);
  });
  live_timer_.setInterval(30);
  connect(&live_timer_, &QTimer::timeout, this, [this] {
    if (!isVisible() || !session_->IsRunning())
      return;
    if (loc_[kSeg7] + 15 < kMemSize)
      session_->RequestMemorySnapshot(loc_[kSeg7], 16);
    session_->RequestMemorySnapshot(loc_[kLeds], 1);
  });
  live_timer_.start();
  // pbAddrChange() at start-up: the buttons read released.
  WriteByte(loc_[kButtons], 0, 0xFF);
}

HardwareWindow::~HardwareWindow() {
  // Fields losing the focus while the children are destroyed must not reach
  // this object.
  for (QLineEdit* e : address_)
    e->disconnect(this);
}

void HardwareWindow::BuildDisplays() {
  segments_ = new SegmentPanel(this);
  segments_->setGeometry(8, 8, 329, 69);
  leds_ = new LedPanel(this);
  leds_->setGeometry(8, 84, 329, 33);
  for (int bit = 0; bit < 8; bit++) {
    const int x = 16 + (7 - bit) * 40;
    Label(QString::number(bit), this, QRect(x + 8, 116, 17, 17));
    auto* sw = new GlyphButton(GlyphButton::kSwitch, QStringLiteral(":/hardware/switch.png"), this);
    sw->setGeometry(x, 132, 29, 45);
    switches_[bit] = sw;
    connect(sw, &QAbstractButton::clicked, this, [this, bit](bool on) {
      const auto mask = static_cast<uint8_t>(1u << bit);
      if (on)
        switch_bits_ |= mask;
      else
        switch_bits_ &= static_cast<uint8_t>(~mask);
      WriteByte(loc_[kSwitches], static_cast<uint8_t>(~mask), on ? mask : 0);
    });
    auto* pb =
        new GlyphButton(GlyphButton::kPush, QStringLiteral(":/hardware/push_button.png"), this);
    pb->setGeometry(x, 192, 33, 33);
    buttons_[bit] = pb;
    const auto mask = static_cast<uint8_t>(1u << bit);
    connect(pb, &QAbstractButton::pressed, this,
            [this, mask] { WriteByte(loc_[kButtons], static_cast<uint8_t>(~mask), 0); });
    connect(pb, &QAbstractButton::released, this,
            [this, mask] { WriteByte(loc_[kButtons], 0xFF, mask); });
  }
}

void HardwareWindow::BuildAddresses() {
  static constexpr int kTops[4] = {32, 80, 136, 184};
  QFont bold = font();
  bold.setPixelSize(13);
  bold.setBold(true);
  const auto addresses = session_->Env().GetHardwareAddresses();
  const uint32_t initial[4] = {addresses.seg7, addresses.leds, addresses.switches,
                               addresses.buttons};
  for (int d = 0; d < 4; d++) {
    Label(QStringLiteral("Address:"), this, QRect(342, kTops[d], 73, 17))->setFont(bold);
    address_[d] = AddressEdit(this, 342, kTops[d] + 16);
    loc_[d] = initial[d];
    address_[d]->setText(QStringLiteral("%1").arg(initial[d], 6, 16, QLatin1Char('0')).toUpper());
  }
  for (int d = 0; d < 4; d++) {
    connect(address_[d], &QLineEdit::textChanged, this,
            [this, d] { OnAddressChanged(static_cast<Device>(d)); });
  }
  // switchAddrExit()/KeyDown(Enter): the switches at the new address;
  // pbAddrExit()/KeyDown(Enter): the buttons released.
  connect(address_[kSwitches], &QLineEdit::editingFinished, this,
          [this] { WriteByte(loc_[kSwitches], 0, switch_bits_); });
  connect(address_[kButtons], &QLineEdit::editingFinished, this,
          [this] { WriteByte(loc_[kButtons], 0, 0xFF); });
  OnAddressChanged(kSeg7);
}

void HardwareWindow::BuildInterrupts() {
  auto* irq_box = Group(QStringLiteral(" Interrupt "), kMaroon, this, QRect(8, 240, 241, 105));
  for (int level = 1; level <= 7; level++) {
    const int x = 8 + (7 - level) * 32;
    auto* label = Label(QString::number(level), irq_box, QRect(x, 16, 25, 17));
    label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    auto* b =
        new GlyphButton(GlyphButton::kSpeed, QStringLiteral(":/hardware/irq_button.png"), irq_box);
    b->setGeometry(x, 32, 25, 25);
    b->setEnabled(session_->HardwareEnabled());
    irq_[level - 1] = b;
    connect(b, &QAbstractButton::clicked, this, [this, level] { session_->Interrupt(level); });
    auto* box = new QCheckBox(irq_box);
    box->setGeometry(x + 8, 64, 17, 17);
    auto_box_[level - 1] = box;
    connect(box, &QCheckBox::clicked, this, [this, level] { ApplyAutoIrq(level); });
  }
  auto_label_ = Label(QStringLiteral("Automatic"), irq_box, QRect(16, 80, 209, 17));
  auto_label_->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  auto* interval_box =
      Group(QStringLiteral("Auto Interval"), kMaroon, this, QRect(248, 240, 113, 105));
  interval_ = new QLineEdit(interval_box);
  interval_->setGeometry(8, 28, 70, 24);
  interval_->setMaxLength(8);
  interval_->setValidator(
      new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[0-9]{0,8}")), interval_));
  connect(interval_, &QLineEdit::inputRejected, this, [] { QApplication::beep(); });
  QFont small = font();
  small.setPixelSize(11);
  Label(QStringLiteral("mS"), interval_box, QRect(81, 34, 18, 13))->setFont(small);
  auto_level_ = new QComboBox(interval_box);
  auto_level_->setGeometry(24, 56, 49, 24);
  for (int level = 1; level <= 7; level++)
    auto_level_->addItem(QString::number(level));
  Label(QStringLiteral(" IRQ"), interval_box, QRect(73, 66, 27, 13))->setFont(small);
  // AutoIRQChange(): the interval of the selected level; IRQintervalChange():
  // a new interval for it.
  connect(auto_level_, &QComboBox::currentIndexChanged, this, &HardwareWindow::ShowAutoIrqs);
  connect(interval_, &QLineEdit::textEdited, this,
          [this] { ApplyAutoIrq(auto_level_->currentIndex() + 1); });

  auto* reset_box = Group(QStringLiteral("Reset"), kMaroon, this, QRect(360, 240, 57, 105));
  reset_ =
      new GlyphButton(GlyphButton::kSpeed, QStringLiteral(":/hardware/irq_button.png"), reset_box);
  reset_->setGeometry(16, 40, 25, 25);
  reset_->setEnabled(session_->HardwareEnabled());
  connect(reset_, &QAbstractButton::clicked, session_, &SimSession::HardwareReset);

  QtSimulatorEnv& env = session_->Env();
  connect(&env, &QtSimulatorEnv::AutoIrqChanged, this, &HardwareWindow::ShowAutoIrqs);
  connect(&env, &QtSimulatorEnv::AutoIrqEnabledChanged, this, [this](bool enabled) {
    auto_label_->setText(enabled ? QStringLiteral("Automatic Enabled")
                                 : QStringLiteral("Automatic Disabled"));
  });
  ShowAutoIrqs();
}

void HardwareWindow::BuildMemoryMap() {
  auto* box = Group(QStringLiteral("Memory Map"), kGray, this, QRect(8, 352, 409, 137));
  auto black = [](QLabel* label) {
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, Qt::black);
    label->setPalette(pal);
    return label;
  };
  black(Label(QStringLiteral("Start:"), box, QRect(112, 16, 73, 15)));
  black(Label(QStringLiteral("End:"), box, QRect(192, 16, 73, 15)));
  static const char* const kNames[4] = {"ROM:", "Read:", "Protected:", "Invalid:"};
  static const char* const kNotes[4] = {"Writes are ignored", "Bus error on write",
                                        "Supervisor access", "Bus error on access"};
  for (int r = 0; r < 4; r++) {
    const int y = 32 + r * 24;
    map_check_[r] = new QCheckBox(box);
    map_check_[r]->setGeometry(8, y, 17, 21);
    auto* name = black(Label(QString::fromLatin1(kNames[r]), box, QRect(25, y, 80, 16)));
    name->setAlignment(Qt::AlignRight | Qt::AlignTop);
    map_start_[r] = AddressEdit(box, 112, y);
    map_end_[r] = AddressEdit(box, 192, y);
    Label(QString::fromLatin1(kNotes[r]), box, QRect(272, y, 121, 21));
    connect(map_check_[r], &QCheckBox::toggled, this, &HardwareWindow::ApplyMemoryMap);
    connect(map_start_[r], &QLineEdit::textChanged, this, &HardwareWindow::ApplyMemoryMap);
    connect(map_end_[r], &QLineEdit::textChanged, this, &HardwareWindow::ApplyMemoryMap);
  }
}

// seg7addrChange() ... pbAddrChange(), addrKeyUp(): the new address is used
// at once; addresses that overlap or leave memory are shown in red.
void HardwareWindow::OnAddressChanged(Device device) {
  loc_[device] = HexValue(address_[device]);
  session_->SetHardwareAddresses(loc_[kSeg7], loc_[kLeds], loc_[kSwitches], loc_[kButtons]);
  for (int d = 0; d < 4; d++) {
    QPalette pal = address_[d]->palette();
    pal.setColor(QPalette::Text, AddressConflict(static_cast<Device>(d)) ? kRed : Qt::black);
    address_[d]->setPalette(pal);
  }
  if (device == kSeg7 || device == kLeds)
    Refresh();
}

// addrKeyUp(). The 7-segment display needs 16 bytes of memory (the original
// accepted addresses up to MEMSIZE-9 and read past the end of memory).
bool HardwareWindow::AddressConflict(Device device) const {
  const uint32_t seg7 = loc_[kSeg7], led = loc_[kLeds], sw = loc_[kSwitches], pb = loc_[kButtons];
  auto in_seg7 = [seg7](uint32_t a) { return a >= seg7 && a <= seg7 + 15; };
  switch (device) {
    case kSeg7:
      return seg7 + 15 >= kMemSize || in_seg7(led) || in_seg7(sw) || in_seg7(pb);
    case kLeds:
      return led >= kMemSize || in_seg7(led) || led == sw || led == pb;
    case kSwitches:
      return sw >= kMemSize || in_seg7(sw) || sw == led || sw == pb;
    case kButtons:
      return pb >= kMemSize || in_seg7(pb) || pb == led || pb == sw;
  }
  return false;
}

bool HardwareWindow::ReadByte(uint32_t address, uint8_t* value) const {
  if (address >= kMemSize)
    return false;
  if (!session_->IsRunning()) {
    *value = session_->Simulator().GetMemory().RawData()[address];
    return true;
  }
  const uint32_t seg7 = loc_[kSeg7];
  if (address >= seg7 && address - seg7 < static_cast<uint32_t>(seg_snapshot_.size())) {
    *value = static_cast<uint8_t>(seg_snapshot_[static_cast<qsizetype>(address - seg7)]);
    return true;
  }
  if (address == loc_[kLeds] && led_snapshot_ >= 0) {
    *value = static_cast<uint8_t>(led_snapshot_);
    return true;
  }
  return false;
}

// update(): digit 7 (left) shows the byte at the 7-segment address, digit 0
// the byte at +14; the LEDs show the LED byte.
void HardwareWindow::Refresh() {
  if (!session_->IsRunning()) {
    seg_snapshot_.clear();
    led_snapshot_ = -1;
  }
  uint8_t value = 0;
  if (loc_[kSeg7] + 15 < kMemSize) {
    for (int digit = 0; digit < 8; digit++) {
      if (ReadByte(loc_[kSeg7] + 2 * static_cast<uint32_t>(7 - digit), &value))
        segments_->SetByte(digit, value);
    }
  }
  if (ReadByte(loc_[kLeds], &value))
    leds_->SetByte(value);
  segments_->update();
  leds_->update();
}

void HardwareWindow::RefreshIfNeeded(uint32_t address) {
  if (address >= kMemSize)
    return;
  const int64_t a = address;
  const int64_t seg7 = loc_[kSeg7], led = loc_[kLeds];
  if ((a >= seg7 - 4 && a <= seg7 + 15) || (a >= led - 4 && a <= led))
    Refresh();
}

void HardwareWindow::Initialize() {
  switch_bits_ = 0;
  for (QAbstractButton* sw : switches_)
    sw->setChecked(false);
  Refresh();
}

int HardwareWindow::SegmentByte(int digit) const {
  return segments_->Byte(digit);
}

int HardwareWindow::LedByte() const {
  return leds_->Byte();
}

void HardwareWindow::WriteByte(uint32_t address, uint8_t and_mask, uint8_t or_mask) {
  if (address >= kMemSize)
    return;
  session_->RunOnSimulator([address, and_mask, or_mask](sim::M68kSimulator& sim) {
    uint8_t* m = sim.GetMemory().RawData();
    m[address] = static_cast<uint8_t>((m[address] & and_mask) | or_mask);
  });
  if (!session_->IsRunning()) {
    emit MemoryChanged();  // MemoryFrm->Repaint(), StackFrm->Repaint()
    Refresh();
  }
}

void HardwareWindow::OnSnapshot(uint32_t address, const QByteArray& bytes) {
  if (address == loc_[kSeg7] && bytes.size() == 16)
    seg_snapshot_ = bytes;
  else if (address == loc_[kLeds] && bytes.size() == 1)
    led_snapshot_ = static_cast<uint8_t>(bytes[0]);
  else
    return;
  Refresh();
}

void HardwareWindow::ShowAutoIrqs() {
  const QtSimulatorEnv& env = session_->Env();
  for (int level = 1; level <= 7; level++)
    auto_box_[level - 1]->setChecked(env.GetAutoIrq(level).checked);
  const int interval = env.GetAutoIrq(auto_level_->currentIndex() + 1).interval_ms;
  bool ok = false;
  if (interval_->text().toInt(&ok) != interval || !ok)
    interval_->setText(QString::number(interval));
}

// CheckBoxNClick() / IRQintervalChange(): the level's check box and interval.
void HardwareWindow::ApplyAutoIrq(int level) {
  QtSimulatorEnv& env = session_->Env();
  QtSimulatorEnv::AutoIrq config = env.GetAutoIrq(level);
  config.checked = auto_box_[level - 1]->isChecked();
  if (level == auto_level_->currentIndex() + 1)
    config.interval_ms = interval_->text().toInt();  // empty = 0: never fires
  env.SetAutoIrq(level, config);
}

// SIMOPS2.CPP loadSrec(): ranges from the program's S0 records.
void HardwareWindow::ShowMemoryMap() {
  const auto& map = session_->MemoryMap();
  const sim::M68kSimulator::MemoryMapConfig::Range* ranges[4] = {&map.rom, &map.read,
                                                                 &map.protected_, &map.invalid};
  showing_map_ = true;
  for (int r = 0; r < 4; r++) {
    map_check_[r]->setChecked(ranges[r]->enabled);
    if (ranges[r]->enabled) {
      map_start_[r]->setText(
          QStringLiteral("%1").arg(ranges[r]->start, 6, 16, QLatin1Char('0')).toUpper());
      map_end_[r]->setText(
          QStringLiteral("%1").arg(ranges[r]->end, 6, 16, QLatin1Char('0')).toUpper());
    }
  }
  showing_map_ = false;
}

// ROMChkClick(), ROMStartEditChange() ...: the map follows the fields.
void HardwareWindow::ApplyMemoryMap() {
  if (showing_map_)
    return;
  sim::M68kSimulator::MemoryMapConfig map;
  sim::M68kSimulator::MemoryMapConfig::Range* ranges[4] = {&map.rom, &map.read, &map.protected_,
                                                           &map.invalid};
  for (int r = 0; r < 4; r++)
    *ranges[r] = {map_check_[r]->isChecked(), HexValue(map_start_[r]), HexValue(map_end_[r])};
  session_->SetMemoryMap(map);
}

// FormShow(): the displays are read and the switch byte written (the
// original wrote 0, also when switches were on).
void HardwareWindow::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  Refresh();
  WriteByte(loc_[kSwitches], 0, switch_bits_);
}

// FormKeyDown(): Ctrl+Tab goes on to the Break Points window.
void HardwareWindow::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Tab && (event->modifiers() & Qt::ControlModifier)) {
    emit NextWindowRequested();
    return;
  }
  QWidget::keyPressEvent(event);
}

}  // namespace easym68k::gui
