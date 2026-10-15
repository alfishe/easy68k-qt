// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// StdFileIO — headless file back end for Trap #15 tasks 50-59, ported from
// the original's SIMOPS2.CPP (closeFiles, openFile, newFile, readFile,
// writeFile, positionFile, closeFile, deleteFile, fileOp) on C stdio. Up to
// 8 files are open at once (MAXFILES); file IDs are the table positions.
//
// Relative file names are resolved against a base directory (the original
// used the process's current directory, normally the program's folder).
// There is no file dialog headless: task 58 reports an error.

#pragma once

#include <array>
#include <cstdio>
#include <string>
#include <utility>

#include "easym68k/sim/ifile_io.h"

namespace easym68k::sim {

// Trap #15 file result codes (def.h).
constexpr short kFileSuccess = 0;
constexpr short kFileEof = 1;
constexpr short kFileError = 2;
constexpr short kFileReadOnly = 3;

class StdFileIO : public IFileIO {
 public:
  static constexpr int kMaxFiles = 8;

  // `base_dir` empty: names are used as given (process current directory).
  explicit StdFileIO(std::string base_dir = "");
  ~StdFileIO() override;

  // Directory for relative names (the GUI uses the loaded program's, as
  // the original's current directory followed its file dialogs). Call while
  // the simulator is not running.
  void SetBaseDir(std::string base_dir) { base_dir_ = std::move(base_dir); }

  StdFileIO(const StdFileIO&) = delete;
  StdFileIO& operator=(const StdFileIO&) = delete;

  void CloseAll(short* result) override;
  void OpenFile(long* fid, const char* name, short* result) override;
  void NewFile(long* fid, const char* name, short* result) override;
  void ReadFile(long fid, char* buf, unsigned int* size, short* result) override;
  void WriteFile(long fid, const char* buf, unsigned int size, short* result) override;
  void PositionFile(long fid, int offset, short* result) override;
  void CloseFile(long fid, short* result) override;
  void DeleteFile(const char* name, short* result) override;
  void DisplayFileDialog(long* mode, long a1, long a2, long a3, short* result) override;
  void FileOp(long* mode, const char* name, short* result) override;

 private:
  std::string Resolve(const char* name) const;
  std::FILE* Get(long fid) const;

  std::string base_dir_;
  std::array<std::FILE*, kMaxFiles> files_{};
};

}  // namespace easym68k::sim
