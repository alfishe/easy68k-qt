// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Listing Generator for EASy68K 68000 Assembler (.L68 output)

#ifndef EASYM68K_ASM_LISTING_GENERATOR_H_
#define EASYM68K_ASM_LISTING_GENERATOR_H_

#include <cstdint>
#include <string>
#include <vector>

namespace easym68k::asm_ {

struct EmittedItem {
  uint32_t value;
  int size;  // 1 = byte, 2 = word, 4 = long
};

class ListingGenerator {
 public:
  ListingGenerator();
  ~ListingGenerator();

  // Reset generator state
  void Reset();

  // Configure listing settings
  void Configure(const std::string& filename, const std::string& date, int page_length = 60,
                 bool cex_flag = true);

  // Set the starting/entry address
  void SetStartAddress(uint32_t addr);

  // Request a page break on next line (or immediately if applicable)
  void AddPageBreak();

  // Add an assembled source line
  void AddLine(uint32_t addr, const std::vector<EmittedItem>& emitted, const std::string& src,
               int src_line, const std::string& ident, bool offset_mode, bool show_equal,
               int32_t equ_val, bool has_equ);

  // Add an error/warning line
  void AddErrorLine(const std::string& message, bool is_warning);

  // Generate complete L68 listing
  std::string Generate(int error_count, int warning_count,
                       const std::string& symbol_table_cre = "") const;

  // Format emitted items into 32-character column blocks
  std::vector<std::string> FormatLeftColumns(uint32_t addr, const std::vector<EmittedItem>& emitted,
                                             bool offset_mode, bool show_equal, int32_t equ_val,
                                             bool has_equ) const;

  // Replaces tabs in source text with spaces matching original smart tab stops
  std::string ExpandTabs(const std::string& text) const;

 private:
  std::string filename_;
  std::string date_str_;
  int page_length_;
  bool cex_flag_;
  uint32_t start_address_;

  struct ListLine {
    std::string left_column;
    std::string right_column;
    bool is_page_break;
    bool is_error;
    bool is_warning;
    bool is_continuation;
  };

  mutable std::vector<ListLine> lines_;
  bool pending_page_break_;
};

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_LISTING_GENERATOR_H_
