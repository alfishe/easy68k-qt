// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Symbol Table for EASy68K 68000 Assembler

#ifndef EASYM68K_ASM_SYMBOL_TABLE_H_
#define EASYM68K_ASM_SYMBOL_TABLE_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace easym68k::asm_ {

enum class SymbolFlags : uint8_t {
  kNone = 0x00,
  kBackRef = 0x01,       // Defined in pass 2 (backward reference)
  kRedefinable = 0x02,   // Defined by SET (can be redefined)
  kRegisterList = 0x04,  // Defined by REG directive
  kMacro = 0x08,         // Macro name
  kDS = 0x10,            // Defined by DS directive
  kLocal = 0x20,         // Local label
  kExternal = 0x40,      // Imported symbol (XREF)
  kPublic = 0x80         // Exported symbol (XDEF)
};

inline SymbolFlags operator|(SymbolFlags a, SymbolFlags b) {
  return static_cast<SymbolFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline SymbolFlags operator&(SymbolFlags a, SymbolFlags b) {
  return static_cast<SymbolFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool HasFlag(SymbolFlags flags, SymbolFlags test) {
  return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(test)) != 0;
}

struct ForwardRef {
  int line_number;
  uint32_t address;
  int size;  // Bytes: 1, 2, or 4
  bool is_relative;

  ForwardRef() : line_number(0), address(0), size(2), is_relative(false) {}
};

struct SymbolInfo {
  std::string name;
  int32_t value;
  SymbolFlags flags;
  int definition_line;
  bool is_defined;
  std::string parent_label;
  std::vector<ForwardRef> forward_refs;

  SymbolInfo() : value(0), flags(SymbolFlags::kNone), definition_line(0), is_defined(false) {}
};

class SymbolTable {
 public:
  SymbolTable();
  ~SymbolTable();

  void Clear();

  // Returns false if name already defined (unless redefinable via SET).
  bool Define(const std::string& name, int32_t value, int line,
              SymbolFlags flags = SymbolFlags::kNone);

  bool DefineEqu(const std::string& name, int32_t value, int line);
  bool DefineSet(const std::string& name, int32_t value, int line);
  bool DefineLabel(const std::string& name, uint32_t address, int line, bool is_ds = false);
  bool DefineRegisterList(const std::string& name, uint16_t reg_mask, int line);
  bool DefineMacro(const std::string& name, int line);

  bool Lookup(const std::string& name, SymbolInfo& info) const;
  bool Exists(const std::string& name) const;
  bool IsDefined(const std::string& name) const;
  int32_t GetValue(const std::string& name) const;

  void AddForwardRef(const std::string& name, int line, uint32_t address, int size,
                     bool is_relative);
  const std::vector<ForwardRef>& GetForwardRefs(const std::string& name) const;
  void ResolveForwardRefs(const std::string& name);

  void SetCurrentParentLabel(const std::string& label);

  std::vector<SymbolInfo> GetAllSymbols() const;
  std::vector<std::string> GetUndefinedSymbols() const;

  void MarkAsBackRef(const std::string& name);

  void SetCaseSensitive(bool sensitive) { case_sensitive_ = sensitive; }

 private:
  std::unordered_map<std::string, SymbolInfo> symbols_;
  std::string current_parent_label_;
  bool case_sensitive_;
  static std::vector<ForwardRef> empty_refs_;

  std::string NormalizeName(const std::string& name) const;
  std::string ResolveLocalName(const std::string& name) const;
};

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_SYMBOL_TABLE_H_
