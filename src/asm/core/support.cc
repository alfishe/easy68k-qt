// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Portable replacements for the Borland run-time library, VCL and C stdio
// facilities used by the original assembler sources (see asm_core.h).

#include <algorithm>
#include <cstdlib>

#include "asm_core.h"

namespace easym68k::asm_::core {

int stricmp(const char* a, const char* b) {
  for (;; ++a, ++b) {
    int ca = std::toupper(static_cast<unsigned char>(*a));
    int cb = std::toupper(static_cast<unsigned char>(*b));
    if (ca != cb || !ca)
      return ca - cb;
  }
}

int strnicmp(const char* a, const char* b, size_t n) {
  for (; n; --n, ++a, ++b) {
    int ca = std::toupper(static_cast<unsigned char>(*a));
    int cb = std::toupper(static_cast<unsigned char>(*b));
    if (ca != cb || !ca)
      return ca - cb;
  }
  return 0;
}

AnsiString AnsiString::UpperCase() const {
  std::string u = s_;
  for (char& c : u)
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return AnsiString(u);
}

AnsiString AnsiString::IntToHex(unsigned int value, int digits) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%0*X", digits, value);
  return AnsiString(buf);
}

// ---------------------------------------------------------------------------
// MemFile stdio
// ---------------------------------------------------------------------------

char* fgets(char* s, int n, MemFile* f) {
  if (!f || n <= 0)
    return nullptr;
  if (f->pos_ >= f->data_.size()) {
    f->eof_ = true;
    return nullptr;
  }
  int i = 0;
  while (i < n - 1 && f->pos_ < f->data_.size()) {
    char c = f->data_[f->pos_++];
    s[i++] = c;
    if (c == '\n')
      break;
  }
  s[i] = '\0';
  return s;
}

int fprintf(MemFile* f, const char* format, ...) {
  if (!f)
    return 0;
  char buf[2048];
  va_list args;
  va_start(args, format);
  int n = std::vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  if (n < 0)
    return n;
  std::string text(buf, std::min<size_t>(static_cast<size_t>(n), sizeof(buf) - 1));
  f->data_.replace(f->pos_, std::min(text.size(), f->data_.size() - f->pos_), text);
  f->pos_ += text.size();
  return n;
}

int fputs(const char* s, MemFile* f) {
  return fprintf(f, "%s", s) < 0 ? EOF : 0;
}

int putc(int c, MemFile* f) {
  char s[2] = {static_cast<char>(c), '\0'};
  fprintf(f, "%s", s);
  return c;
}

long ftell(MemFile* f) {
  return f ? static_cast<long>(f->pos_) : -1;
}

int fseek(MemFile* f, long offset, int origin) {
  if (!f)
    return -1;
  long base = origin == SEEK_SET ? 0
              : origin == SEEK_CUR ? static_cast<long>(f->pos_)
                                   : static_cast<long>(f->data_.size());
  long pos = base + offset;
  if (pos < 0)
    return -1;
  f->pos_ = static_cast<size_t>(pos);
  if (f->pos_ > f->data_.size())
    f->data_.resize(f->pos_, '\0');
  f->eof_ = false;
  return 0;
}

void rewind(MemFile* f) {
  if (f) {
    f->pos_ = 0;
    f->eof_ = false;
  }
}

int feof(MemFile* f) {
  return f && f->eof_;
}

int ferror(MemFile*) {
  return 0;
}

size_t fread(void* ptr, size_t size, size_t count, MemFile* f) {
  if (!f || size == 0)
    return 0;
  size_t done = 0;
  char* out = static_cast<char*>(ptr);
  while (done < count) {
    if (f->pos_ + size > f->data_.size()) {
      f->eof_ = true;
      break;
    }
    std::memcpy(out + done * size, f->data_.data() + f->pos_, size);
    f->pos_ += size;
    ++done;
  }
  return done;
}

// ---------------------------------------------------------------------------
// AsmCore
// ---------------------------------------------------------------------------

AsmCore::AsmCore(CoreOptions options, CoreFileReader reader)
    : options_(std::move(options)), reader_(std::move(reader)) {}

AsmCore::~AsmCore() {
  clearSymbols();
}

MemFile* AsmCore::NewFile(std::string data) {
  files_.push_back(std::make_unique<MemFile>(std::move(data)));
  return files_.back().get();
}

MemFile* AsmCore::OpenFile(const char* name, const char* operand) {
  std::string contents;
  if (!reader_)
    return nullptr;
  // The file name as written in the source (same characters as `name`,
  // which strcap() capitalized outside single quotes).
  std::string written = name;
  if (curLine_ && curCapLine_ && operand >= curCapLine_ &&
      operand < curCapLine_ + std::strlen(curCapLine_)) {
    const char* raw = curLine_ + (operand - curCapLine_);
    std::string op = raw;
    size_t start = op.find_first_not_of(" \t");
    if (start != std::string::npos) {
      op = op.substr(start);
      char quote = (op[0] == '"' || op[0] == '\'') ? op[0] : '\0';
      if (quote)
        op = op.substr(1);
      size_t end = 0;
      while (end < op.size() && op[end] != '\n' && (op[end] != ' ' || quote) && op[end] != quote)
        ++end;
      op = op.substr(0, end);
      while (!op.empty() && std::isspace(static_cast<unsigned char>(op.back())))
        op.pop_back();
      if (stricmp(op.c_str(), name) == 0)
        written = op;
    }
  }
  if (reader_(written, &contents) || (written != name && reader_(name, &contents)))
    return NewFile(std::move(contents));
  return nullptr;
}

}  // namespace easym68k::asm_::core
