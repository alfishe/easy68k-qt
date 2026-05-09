// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ISerialIO — serial port I/O interface (Trap #15 tasks 40-43).
// Replaces Win32 HANDLE/COMMTIMEOUTS serial API in original CODE9.CPP.

#pragma once

namespace easym68k::sim {

class ISerialIO {
 public:
  virtual ~ISerialIO() = default;
  virtual void InitPort(int cid, const char* port_name, short* result) = 0;
  virtual void SetPortParams(int cid, int settings, short* result) = 0;
  virtual void ReadString(int cid, unsigned char* count, char* buf, short* result) = 0;
  virtual void SendString(int cid, unsigned char* count, const char* buf, short* result) = 0;
  virtual void CloseAll() = 0;
};

}  // namespace easym68k::sim
