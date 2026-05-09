// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/symbol_table.h"

#include <algorithm>
#include <cctype>

namespace easym68k::asm_ {

std::vector<ForwardRef> SymbolTable::empty_refs_;

SymbolTable::SymbolTable() : case_sensitive_(false) {}

SymbolTable::~SymbolTable() = default;

void SymbolTable::Clear() {
  symbols_.clear();
  current_parent_label_.clear();
}

std::string SymbolTable::NormalizeName(const std::string& name) const {
  if (case_sensitive_)
    return name;
  std::string result = name;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

std::string SymbolTable::ResolveLocalName(const std::string& name) const {
  if (!name.empty() && name[0] == '.') {
    return current_parent_label_ + name;
  }
  return name;
}

bool SymbolTable::Define(const std::string& name, int32_t value, int line, SymbolFlags flags) {
  std::string resolved = ResolveLocalName(name);
  std::string key = NormalizeName(resolved);

  auto it = symbols_.find(key);
  if (it != symbols_.end()) {
    if (HasFlag(it->second.flags, SymbolFlags::kRedefinable)) {
      it->second.value = value;
      it->second.definition_line = line;
      it->second.is_defined = true;
      return true;
    }
    return false;
  }

  SymbolInfo info;
  info.name = resolved;
  info.value = value;
  info.flags = flags;
  info.definition_line = line;
  info.is_defined = true;

  if (!name.empty() && name[0] == '.') {
    info.flags = info.flags | SymbolFlags::kLocal;
    info.parent_label = current_parent_label_;
  }

  symbols_[key] = info;
  return true;
}

bool SymbolTable::DefineEqu(const std::string& name, int32_t value, int line) {
  return Define(name, value, line, SymbolFlags::kNone);
}

bool SymbolTable::DefineSet(const std::string& name, int32_t value, int line) {
  std::string resolved = ResolveLocalName(name);
  std::string key = NormalizeName(resolved);

  auto it = symbols_.find(key);
  if (it != symbols_.end()) {
    if (HasFlag(it->second.flags, SymbolFlags::kRedefinable)) {
      it->second.value = value;
      it->second.definition_line = line;
      return true;
    }
    return false;
  }

  return Define(name, value, line, SymbolFlags::kRedefinable);
}

bool SymbolTable::DefineLabel(const std::string& name, uint32_t address, int line, bool is_ds) {
  SymbolFlags flags = SymbolFlags::kNone;
  if (is_ds)
    flags = flags | SymbolFlags::kDS;

  if (name.empty() || name[0] != '.') {
    current_parent_label_ = name;
  }

  return Define(name, static_cast<int32_t>(address), line, flags);
}

bool SymbolTable::DefineRegisterList(const std::string& name, uint16_t reg_mask, int line) {
  return Define(name, static_cast<int32_t>(reg_mask), line, SymbolFlags::kRegisterList);
}

bool SymbolTable::DefineMacro(const std::string& name, int line) {
  return Define(name, 0, line, SymbolFlags::kMacro);
}

bool SymbolTable::Lookup(const std::string& name, SymbolInfo& info) const {
  std::string resolved = ResolveLocalName(name);
  std::string key = NormalizeName(resolved);

  auto it = symbols_.find(key);
  if (it != symbols_.end()) {
    info = it->second;
    return true;
  }
  return false;
}

bool SymbolTable::Exists(const std::string& name) const {
  std::string resolved = ResolveLocalName(name);
  std::string key = NormalizeName(resolved);
  return symbols_.find(key) != symbols_.end();
}

bool SymbolTable::IsDefined(const std::string& name) const {
  SymbolInfo info;
  return Lookup(name, info) && info.is_defined;
}

int32_t SymbolTable::GetValue(const std::string& name) const {
  SymbolInfo info;
  if (Lookup(name, info))
    return info.value;
  return 0;
}

void SymbolTable::AddForwardRef(const std::string& name, int line, uint32_t address, int size,
                                bool is_relative) {
  std::string resolved = ResolveLocalName(name);
  std::string key = NormalizeName(resolved);

  if (symbols_.find(key) == symbols_.end()) {
    SymbolInfo info;
    info.name = resolved;
    info.value = 0;
    info.flags = SymbolFlags::kNone;
    info.is_defined = false;
    symbols_[key] = info;
  }

  ForwardRef ref;
  ref.line_number = line;
  ref.address = address;
  ref.size = size;
  ref.is_relative = is_relative;
  symbols_[key].forward_refs.push_back(ref);
}

const std::vector<ForwardRef>& SymbolTable::GetForwardRefs(const std::string& name) const {
  std::string resolved = ResolveLocalName(name);
  std::string key = NormalizeName(resolved);

  auto it = symbols_.find(key);
  if (it != symbols_.end())
    return it->second.forward_refs;
  return empty_refs_;
}

void SymbolTable::ResolveForwardRefs(const std::string& name) {
  std::string resolved = ResolveLocalName(name);
  std::string key = NormalizeName(resolved);

  auto it = symbols_.find(key);
  if (it != symbols_.end())
    it->second.forward_refs.clear();
}

void SymbolTable::SetCurrentParentLabel(const std::string& label) {
  current_parent_label_ = label;
}

std::vector<SymbolInfo> SymbolTable::GetAllSymbols() const {
  std::vector<SymbolInfo> result;
  result.reserve(symbols_.size());
  for (const auto& pair : symbols_)
    result.push_back(pair.second);
  std::sort(result.begin(), result.end(),
            [](const SymbolInfo& a, const SymbolInfo& b) { return a.name < b.name; });
  return result;
}

std::vector<std::string> SymbolTable::GetUndefinedSymbols() const {
  std::vector<std::string> result;
  for (const auto& pair : symbols_) {
    if (!pair.second.is_defined)
      result.push_back(pair.second.name);
  }
  std::sort(result.begin(), result.end());
  return result;
}

void SymbolTable::MarkAsBackRef(const std::string& name) {
  std::string resolved = ResolveLocalName(name);
  std::string key = NormalizeName(resolved);

  auto it = symbols_.find(key);
  if (it != symbols_.end()) {
    it->second.flags = it->second.flags | SymbolFlags::kBackRef;
  }
}

}  // namespace easym68k::asm_
