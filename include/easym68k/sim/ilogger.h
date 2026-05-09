// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ILogger — abstract logging interface for simulator diagnostics.
// Replaces direct VCL Form1->Message->Lines->Add() calls in the original.

#pragma once

#include <string_view>

namespace easym68k::sim {

class ILogger {
 public:
  virtual ~ILogger() = default;
  virtual void Log(std::string_view message) = 0;
};

}  // namespace easym68k::sim
