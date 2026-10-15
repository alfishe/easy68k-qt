// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Port of SIMOPS2.CPP file operations. Differences from the original (all
// original bugs or inefficiencies, see PROGRESS.md 10.4):
//  - fileOp() closed a NULL FILE* when the file did not exist;
//  - openFile()/newFile() retried the open for every free slot after a
//    failure; the port tries once, in the first free slot;
//  - readFile() left D2 at the requested size when nothing was read (EOF or
//    error); the port reports 0 bytes read;
//  - writeFile() reported a zero-length write as an error.

#include "easym68k/sim/std_file_io.h"

#include <utility>

namespace easym68k::sim {

StdFileIO::StdFileIO(std::string base_dir) : base_dir_(std::move(base_dir)) {}

StdFileIO::~StdFileIO() {
  short result;
  CloseAll(&result);
}

std::string StdFileIO::Resolve(const char* name) const {
  std::string n = name ? name : "";
  bool absolute = !n.empty() &&
                  (n[0] == '/' || n[0] == '\\' || (n.size() > 1 && n[1] == ':'));  // C:\..., C:/...
  if (base_dir_.empty() || absolute)
    return n;
  char last = base_dir_.back();
  return (last == '/' || last == '\\') ? base_dir_ + n : base_dir_ + "/" + n;
}

std::FILE* StdFileIO::Get(long fid) const {
  if (fid < 0 || fid >= kMaxFiles)
    return nullptr;
  return files_[static_cast<size_t>(fid)];
}

void StdFileIO::CloseAll(short* result) {
  *result = kFileSuccess;
  for (auto& fp : files_) {
    if (fp != nullptr) {
      if (std::fclose(fp) == EOF)
        *result = kFileError;
      fp = nullptr;
    }
  }
}

// mode 0: does the file exist (and may it be written)? Other modes reserved.
void StdFileIO::FileOp(long* /*mode*/, const char* name, short* result) {
  *result = kFileError;
  std::string path = Resolve(name);
  if (std::FILE* fp = std::fopen(path.c_str(), "r+b")) {
    *result = kFileSuccess;
    std::fclose(fp);
  } else if ((fp = std::fopen(path.c_str(), "rb")) != nullptr) {
    *result = kFileReadOnly;
    std::fclose(fp);
  }
}

void StdFileIO::OpenFile(long* fid, const char* name, short* result) {
  *result = kFileError;
  *fid = -1;
  for (int i = 0; i < kMaxFiles; i++) {
    if (files_[i] != nullptr)
      continue;
    std::string path = Resolve(name);
    if ((files_[i] = std::fopen(path.c_str(), "r+b")) != nullptr) {
      *result = kFileSuccess;
      *fid = i;
    } else if ((files_[i] = std::fopen(path.c_str(), "rb")) != nullptr) {
      *result = kFileReadOnly;
      *fid = i;
    }
    break;  // the first free slot decides
  }
}

void StdFileIO::NewFile(long* fid, const char* name, short* result) {
  *result = kFileError;
  *fid = -1;
  for (int i = 0; i < kMaxFiles; i++) {
    if (files_[i] != nullptr)
      continue;
    if ((files_[i] = std::fopen(Resolve(name).c_str(), "w+b")) != nullptr) {
      *result = kFileSuccess;
      *fid = i;
    }
    break;
  }
}

void StdFileIO::ReadFile(long fid, char* buf, unsigned int* size, short* result) {
  *result = kFileError;
  std::FILE* fp = Get(fid);
  if (fp == nullptr)
    return;
  size_t r = std::fread(buf, 1, *size, fp);
  if (r < 1)
    *result = std::feof(fp) ? kFileEof : kFileError;
  else
    *result = kFileSuccess;
  *size = static_cast<unsigned int>(r);  // bytes read (0 at EOF)
}

void StdFileIO::WriteFile(long fid, const char* buf, unsigned int size, short* result) {
  *result = kFileError;
  std::FILE* fp = Get(fid);
  if (fp == nullptr)
    return;
  if (size == 0 || std::fwrite(buf, size, 1, fp) >= 1)
    *result = kFileSuccess;
}

void StdFileIO::PositionFile(long fid, int offset, short* result) {
  *result = kFileError;
  std::FILE* fp = Get(fid);
  if (fp == nullptr)
    return;
  if (std::fseek(fp, offset, SEEK_SET) == 0)
    *result = kFileSuccess;
}

void StdFileIO::CloseFile(long fid, short* result) {
  *result = kFileError;
  std::FILE* fp = Get(fid);
  if (fp == nullptr)
    return;
  if (std::fclose(fp) == 0)
    *result = kFileSuccess;
  files_[static_cast<size_t>(fid)] = nullptr;
}

void StdFileIO::DeleteFile(const char* name, short* result) {
  *result = std::remove(Resolve(name).c_str()) == 0 ? kFileSuccess : kFileError;
}

void StdFileIO::DisplayFileDialog(long* /*mode*/, long, long, long, short* result) {
  *result = kFileError;  // no dialog without a GUI
}

}  // namespace easym68k::sim
