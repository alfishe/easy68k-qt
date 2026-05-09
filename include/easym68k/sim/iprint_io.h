// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// IPrintIO — printer output interface (Trap #15 task 10).
// Replaces VCL TPrinter in original CODE9.CPP.

#pragma once

namespace easym68k::sim {

class IPrintIO {
 public:
  virtual ~IPrintIO() = default;
  virtual void PrintChar(char ch) = 0;
  virtual void FormFeed() = 0;
};

}  // namespace easym68k::sim
