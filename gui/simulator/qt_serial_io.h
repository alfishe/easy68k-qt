// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ISerialIO for Sim68K-Qt (Trap #15 tasks 40-43) on Qt SerialPort, after
// simIOu.cpp initComm(), setCommParams(), readComm(), sendComm(): 16 ports
// by user ID (CID), default 9600,N,8,1, the original's short timeouts (a
// read returns what arrives within about n+1 ms). Used on the simulator
// thread only (blocking calls, no event loop needed).
//
// Results: 0 success, 1 invalid CID, 2 error, 3 port not initialised,
// 4 timeout. Without Qt SerialPort (EASYM68K_HAS_SERIALPORT unset) every
// port fails to open (2).

#ifndef EASY68K_GUI_SIMULATOR_QT_SERIAL_IO_H_
#define EASY68K_GUI_SIMULATOR_QT_SERIAL_IO_H_

#include <array>
#include <memory>

#include "easym68k/sim/iserial_io.h"

class QSerialPort;

namespace easym68k::gui {

class QtSerialIO : public sim::ISerialIO {
 public:
  static constexpr int kMaxComm = 16;  // def.h MAX_COMM

  QtSerialIO();
  ~QtSerialIO() override;

  QtSerialIO(const QtSerialIO&) = delete;
  QtSerialIO& operator=(const QtSerialIO&) = delete;

  // Opens `port_name` (COM4, ttyUSB0, /dev/ttyS0, ...) as `cid`; an open
  // CID is closed and reused.
  void InitPort(int cid, const char* port_name, short* result) override;
  // Bits 0-7 speed (0 = 9600, 1 = 110, 2 = 300, 3 = 600, 4 = 1200, 5 = 2400,
  // 6 = 4800, 7 = 9600, 8 = 19200, 9 = 38400, 10 = 56000, 11 = 57600,
  // 12 = 115200, 13 = 128000, 14 = 256000), bits 8-9 parity (none, odd,
  // even, mark), bits 10-11 data bits (8, 7, 6), bit 12 two stop bits.
  void SetPortParams(int cid, int settings, short* result) override;
  // Reads up to *count bytes into `buf` (at least *count + 1 bytes), NUL
  // terminated; *count = bytes read (4 if fewer than asked).
  void ReadString(int cid, unsigned char* count, char* buf, short* result) override;
  void SendString(int cid, unsigned char* count, const char* buf, short* result) override;
  void CloseAll() override;

 private:
  struct Port;
  short Check(int cid) const;  // 0, 1 (invalid CID) or 3 (not open)

  std::array<std::unique_ptr<Port>, kMaxComm> ports_;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_QT_SERIAL_IO_H_
