// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Minimal Borland VCL compatibility layer used ONLY to build the original
// EASy68K Edit68K assembler sources on Linux as a reference oracle
// (tools/reference). It implements exactly the subset of AnsiString and the
// handful of GUI objects those sources touch; GUI calls become no-ops or
// stderr messages. Not used by the EASy68K-Qt applications.

#ifndef EASY68K_REFERENCE_VCL_H_
#define EASY68K_REFERENCE_VCL_H_

#include <strings.h>

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stack>
#include <string>
#include <vector>

// The Borland headers made these visible globally.
using std::stack;
using std::vector;

// Borland C runtime string helpers.
inline int stricmp(const char* a, const char* b) {
  return strcasecmp(a, b);
}
inline int strcmpi(const char* a, const char* b) {
  return strcasecmp(a, b);
}
inline int strnicmp(const char* a, const char* b, size_t n) {
  return strncasecmp(a, b, n);
}
inline char* itoa(int value, char* buf, int radix) {
  if (radix == 10) {
    std::sprintf(buf, "%d", value);
  } else {
    char tmp[40];
    int i = 0;
    unsigned v = static_cast<unsigned>(value);
    do {
      int d = v % radix;
      tmp[i++] = static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10);
      v /= radix;
    } while (v);
    int j = 0;
    while (i)
      buf[j++] = tmp[--i];
    buf[j] = '\0';
  }
  return buf;
}

// ---------------------------------------------------------------------------
// AnsiString (1-based indexing, as in the VCL)
// ---------------------------------------------------------------------------
class AnsiString {
 public:
  AnsiString() = default;
  AnsiString(const char* s) : s_(s ? s : "") {}
  AnsiString(const std::string& s) : s_(s) {}
  AnsiString(char c) : s_(1, c) {}
  AnsiString(int v) : s_(std::to_string(v)) {}
  AnsiString(unsigned v) : s_(std::to_string(v)) {}
  AnsiString(long v) : s_(std::to_string(v)) {}

  const char* c_str() const { return s_.c_str(); }
  int Length() const { return static_cast<int>(s_.size()); }
  bool IsEmpty() const { return s_.empty(); }

  AnsiString SubString(int index, int count) const {
    if (index < 1)
      index = 1;
    if (index > Length() || count <= 0)
      return AnsiString();
    return AnsiString(s_.substr(index - 1, count));
  }
  int Pos(const AnsiString& sub) const {
    size_t p = s_.find(sub.s_);
    return p == std::string::npos ? 0 : static_cast<int>(p) + 1;
  }
  AnsiString UpperCase() const {
    std::string r = s_;
    for (auto& c : r)
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return r;
  }
  AnsiString LowerCase() const {
    std::string r = s_;
    for (auto& c : r)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
  }
  AnsiString Trim() const {
    size_t b = s_.find_first_not_of(" \t\r\n");
    if (b == std::string::npos)
      return AnsiString();
    size_t e = s_.find_last_not_of(" \t\r\n");
    return s_.substr(b, e - b + 1);
  }
  AnsiString TrimRight() const {
    size_t e = s_.find_last_not_of(" \t\r\n");
    return e == std::string::npos ? AnsiString() : AnsiString(s_.substr(0, e + 1));
  }
  AnsiString TrimLeft() const {
    size_t b = s_.find_first_not_of(" \t\r\n");
    return b == std::string::npos ? AnsiString() : AnsiString(s_.substr(b));
  }
  AnsiString& Insert(const AnsiString& str, int index) {
    s_.insert(index < 1 ? 0 : index - 1, str.s_);
    return *this;
  }
  AnsiString& Delete(int index, int count) {
    if (index >= 1 && index <= Length())
      s_.erase(index - 1, count);
    return *this;
  }
  int ToInt() const { return static_cast<int>(std::strtol(s_.c_str(), nullptr, 0)); }
  int AnsiCompareIC(const AnsiString& o) const { return strcasecmp(s_.c_str(), o.s_.c_str()); }
  int AnsiCompare(const AnsiString& o) const { return std::strcmp(s_.c_str(), o.s_.c_str()); }

  static AnsiString IntToHex(int value, int digits) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%0*X", digits, static_cast<unsigned>(value));
    return buf;
  }
  AnsiString& sprintf(const char* fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    s_ = buf;
    return *this;
  }

  char& operator[](int i) { return s_[i - 1]; }
  char operator[](int i) const { return s_[i - 1]; }

  AnsiString& operator+=(const AnsiString& o) {
    s_ += o.s_;
    return *this;
  }
  friend AnsiString operator+(const AnsiString& a, const AnsiString& b) { return a.s_ + b.s_; }
  friend AnsiString operator+(const char* a, const AnsiString& b) { return std::string(a) + b.s_; }
  friend AnsiString operator+(const AnsiString& a, const char* b) { return a.s_ + std::string(b); }
  friend bool operator==(const AnsiString& a, const AnsiString& b) { return a.s_ == b.s_; }
  friend bool operator!=(const AnsiString& a, const AnsiString& b) { return a.s_ != b.s_; }
  friend bool operator<(const AnsiString& a, const AnsiString& b) { return a.s_ < b.s_; }
  friend bool operator>(const AnsiString& a, const AnsiString& b) { return a.s_ > b.s_; }
  friend bool operator<=(const AnsiString& a, const AnsiString& b) { return a.s_ <= b.s_; }
  friend bool operator>=(const AnsiString& a, const AnsiString& b) { return a.s_ >= b.s_; }

  const std::string& str() const { return s_; }

 private:
  std::string s_;
};
using String = AnsiString;

inline AnsiString IntToStr(int v) {
  return AnsiString(v);
}
inline AnsiString IntToHex(int v, int digits) {
  return AnsiString::IntToHex(v, digits);
}
inline int StrToInt(const AnsiString& s) {
  return s.ToInt();
}
inline AnsiString UpperCase(const AnsiString& s) {
  return s.UpperCase();
}
inline AnsiString LowerCase(const AnsiString& s) {
  return s.LowerCase();
}
inline AnsiString Trim(const AnsiString& s) {
  return s.Trim();
}

// File name helpers (Windows semantics are not needed for the oracle).
inline AnsiString ExtractFilePath(const AnsiString& f) {
  size_t p = f.str().find_last_of("/\\");
  return p == std::string::npos ? AnsiString() : AnsiString(f.str().substr(0, p + 1));
}
inline AnsiString ExtractFileName(const AnsiString& f) {
  size_t p = f.str().find_last_of("/\\");
  return p == std::string::npos ? f : AnsiString(f.str().substr(p + 1));
}
inline AnsiString ExtractFileExt(const AnsiString& f) {
  AnsiString name = ExtractFileName(f);
  size_t p = name.str().find_last_of('.');
  return p == std::string::npos ? AnsiString() : AnsiString(name.str().substr(p));
}
inline AnsiString ChangeFileExt(const AnsiString& f, const AnsiString& ext) {
  std::string s = f.str();
  size_t slash = s.find_last_of("/\\");
  size_t dot = s.find_last_of('.');
  if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
    s.erase(dot);
  return s + ext.str();
}
inline AnsiString ExpandFileName(const AnsiString& f) {
  return f;
}
inline bool FileExists(const AnsiString& f) {
  FILE* fp = std::fopen(f.c_str(), "r");
  if (fp)
    std::fclose(fp);
  return fp != nullptr;
}
inline bool DeleteFile(const AnsiString& f) {
  return std::remove(f.c_str()) == 0;
}

// Deterministic timestamp for reproducible listings; override with
// EASY68K_REF_DATE.
struct TDateTime {};
inline TDateTime Now() {
  return {};
}
inline AnsiString DateTimeToStr(TDateTime) {
  const char* d = std::getenv("EASY68K_REF_DATE");
  return d ? d : "";
}

// ---------------------------------------------------------------------------
// Colours / misc types referenced by asm.h
// ---------------------------------------------------------------------------
typedef int TColor;
const TColor clBlack = 0x000000, clOlive = 0x008080, clGreen = 0x008000, clBlue = 0xFF0000,
             clPurple = 0x800080, clMaroon = 0x000080, clRed = 0x0000FF, clTeal = 0x808000,
             clWhite = 0xFFFFFF;
const int MB_OK = 0;
typedef void TObject;

// ---------------------------------------------------------------------------
// GUI stand-ins
// ---------------------------------------------------------------------------
struct TApplication {
  int MessageBox(const char* text, const char* caption, int) {
    std::fprintf(stderr, "[%s] %s\n", caption ? caption : "", text ? text : "");
    return 0;
  }
  void ProcessMessages() {}
};
extern TApplication* Application;

inline void ShowMessage(const AnsiString& s) {
  std::fprintf(stderr, "%s\n", s.c_str());
}

struct TMsgDlgButtons {
  TMsgDlgButtons& operator<<(int) { return *this; }
};
enum { mbOK, mbCancel, mbYes, mbNo };
enum { mtError, mtInformation, mtWarning, mtConfirmation };
inline int MessageDlg(const AnsiString& s, int, TMsgDlgButtons, int) {
  std::fprintf(stderr, "%s\n", s.c_str());
  return 0;
}

#endif  // EASY68K_REFERENCE_VCL_H_
