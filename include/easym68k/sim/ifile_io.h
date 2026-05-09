// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// IFileIO — file I/O interface (Trap #15 tasks 50-59).
// Replaces Win32 file handle operations in original CODE9.CPP.

#pragma once

namespace easym68k::sim {

class IFileIO {
 public:
  virtual ~IFileIO() = default;
  virtual void CloseAll(short* result) = 0;
  virtual void OpenFile(long* fid, const char* name, short* result) = 0;
  virtual void NewFile(long* fid, const char* name, short* result) = 0;
  virtual void ReadFile(long fid, char* buf, unsigned int* size, short* result) = 0;
  virtual void WriteFile(long fid, const char* buf, unsigned int size, short* result) = 0;
  virtual void PositionFile(long fid, int offset, short* result) = 0;
  virtual void CloseFile(long fid, short* result) = 0;
  virtual void DeleteFile(const char* name, short* result) = 0;
  virtual void DisplayFileDialog(long* mode, long a1, long a2, long a3, short* result) = 0;
  virtual void FileOp(long* mode, const char* name, short* result) = 0;
};

}  // namespace easym68k::sim
