// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "qt_serial_io.h"

#include <algorithm>
#include <cstring>
#include <QElapsedTimer>
#include <QString>

#ifdef EASYM68K_HAS_SERIALPORT
#include <QSerialPort>
#endif

namespace easym68k::gui {

#ifdef EASYM68K_HAS_SERIALPORT

struct QtSerialIO::Port {
  QSerialPort serial;
  int timeout_ms = 27;  // per-character timeout of the speed (setCommParams)
};

namespace {

// simIOu.cpp setCommParams(): baud rate and per-character timeout (ms).
struct Speed {
  qint32 baud;
  int timeout;
};
Speed SpeedFor(int code) {
  switch (code) {
    case 1:
      return {110, 73};
    case 2:
      return {300, 27};
    case 3:
      return {600, 14};
    case 4:
      return {1200, 7};
    case 5:
      return {2400, 4};
    case 6:
      return {4800, 2};
    case 7:
      return {9600, 1};
    case 8:
      return {19200, 1};
    case 9:
      return {38400, 1};
    case 10:
      return {56000, 1};
    case 11:
      return {57600, 1};
    case 12:
      return {115200, 1};
    case 13:
      return {128000, 1};
    case 14:
      return {256000, 1};
    default:
      return {9600, 27};  // 0 and undefined codes: the default 9600
  }
}

}  // namespace

QtSerialIO::QtSerialIO() = default;
QtSerialIO::~QtSerialIO() {
  CloseAll();
}

short QtSerialIO::Check(int cid) const {
  if (cid < 0 || cid >= kMaxComm)
    return 1;
  return ports_[cid] ? 0 : 3;
}

void QtSerialIO::InitPort(int cid, const char* port_name, short* result) {
  if (cid < 0 || cid >= kMaxComm) {
    *result = 1;
    return;
  }
  ports_[cid].reset();  // close and reuse
  auto port = std::make_unique<Port>();
  port->serial.setPortName(QString::fromLocal8Bit(port_name));
  if (!port->serial.open(QIODevice::ReadWrite)) {
    *result = 2;
    return;
  }
  ports_[cid] = std::move(port);
  SetPortParams(cid, 0, result);
}

void QtSerialIO::SetPortParams(int cid, int settings, short* result) {
  *result = Check(cid);
  if (*result != 0)
    return;
  Port& p = *ports_[cid];
  const Speed speed = SpeedFor(settings & 0xFF);
  p.timeout_ms = speed.timeout;
  static const QSerialPort::Parity kParity[] = {QSerialPort::NoParity, QSerialPort::OddParity,
                                                QSerialPort::EvenParity, QSerialPort::MarkParity};
  static const QSerialPort::DataBits kData[] = {QSerialPort::Data8, QSerialPort::Data7,
                                                QSerialPort::Data6, QSerialPort::Data8};
  const bool ok =
      p.serial.setBaudRate(speed.baud) && p.serial.setParity(kParity[(settings >> 8) & 3]) &&
      p.serial.setDataBits(kData[(settings >> 10) & 3]) &&
      p.serial.setStopBits((settings & 0x1000) ? QSerialPort::TwoStop : QSerialPort::OneStop) &&
      p.serial.setFlowControl(QSerialPort::NoFlowControl);
  if (!ok)
    *result = 2;
  p.serial.clearError();
}

// simIOu.cpp readComm(): ReadIntervalTimeout = the speed's timeout,
// ReadTotalTimeout = 1 + n ms. Exactly the bytes read and a terminator are
// stored (the original copied its buffer up to a stale terminator).
void QtSerialIO::ReadString(int cid, unsigned char* count, char* buf, short* result) {
  *result = Check(cid);
  if (*result != 0)
    return;
  Port& p = *ports_[cid];
  p.serial.clearError();
  const int wanted = *count;
  const int total_ms = 1 + wanted;
  QByteArray data;
  QElapsedTimer t;
  t.start();
  for (;;) {
    data += p.serial.read(wanted - data.size());
    if (data.size() >= wanted)
      break;
    qint64 wait = total_ms - t.elapsed();
    if (!data.isEmpty())
      wait = std::min<qint64>(wait, p.timeout_ms);  // gap between characters
    if (!p.serial.waitForReadyRead(static_cast<int>(std::max<qint64>(wait, 0))))
      break;
  }
  if (p.serial.error() != QSerialPort::NoError && p.serial.error() != QSerialPort::TimeoutError) {
    p.serial.clearError();
    *result = 2;
    return;
  }
  p.serial.clearError();
  std::memcpy(buf, data.constData(), static_cast<size_t>(data.size()));
  buf[data.size()] = '\0';
  if (data.size() < wanted)
    *result = 4;
  *count = static_cast<unsigned char>(data.size());
}

// simIOu.cpp sendComm(): WriteTotalTimeout = 100 + timeout * n ms.
void QtSerialIO::SendString(int cid, unsigned char* count, const char* buf, short* result) {
  *result = Check(cid);
  if (*result != 0)
    return;
  Port& p = *ports_[cid];
  p.serial.clearError();
  const int n = *count;
  if (p.serial.write(buf, n) < 0) {
    *result = 2;
    return;
  }
  QElapsedTimer t;
  t.start();
  const int limit = 100 + p.timeout_ms * n;
  while (p.serial.bytesToWrite() > 0 && t.elapsed() < limit)
    p.serial.waitForBytesWritten(static_cast<int>(limit - t.elapsed()));
  const auto pending = static_cast<int>(p.serial.bytesToWrite());
  if (pending > 0) {
    p.serial.clear(QSerialPort::Output);  // not sent in time
    *result = 4;
  }
  *count = static_cast<unsigned char>(n - pending);
}

void QtSerialIO::CloseAll() {
  for (auto& port : ports_)
    port.reset();
}

#else  // no Qt SerialPort

struct QtSerialIO::Port {};

QtSerialIO::QtSerialIO() = default;
QtSerialIO::~QtSerialIO() = default;

short QtSerialIO::Check(int cid) const {
  return (cid < 0 || cid >= kMaxComm) ? 1 : 3;
}

void QtSerialIO::InitPort(int cid, const char*, short* result) {
  *result = (cid < 0 || cid >= kMaxComm) ? 1 : 2;
}
void QtSerialIO::SetPortParams(int cid, int, short* result) {
  *result = Check(cid);
}
void QtSerialIO::ReadString(int cid, unsigned char*, char*, short* result) {
  *result = Check(cid);
}
void QtSerialIO::SendString(int cid, unsigned char*, const char*, short* result) {
  *result = Check(cid);
}
void QtSerialIO::CloseAll() {}

#endif

}  // namespace easym68k::gui
