// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Listing Generator for EASy68K 68000 Assembler (.L68 output)

#include "easym68k/asm/listing_generator.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace easym68k::asm_ {

ListingGenerator::ListingGenerator()
    : page_length_(60), cex_flag_(true), start_address_(0), pending_page_break_(false) {}

ListingGenerator::~ListingGenerator() = default;

void ListingGenerator::Reset() {
  lines_.clear();
  pending_page_break_ = false;
  start_address_ = 0;
}

void ListingGenerator::Configure(const std::string& filename, const std::string& date,
                                 int page_length, bool cex_flag) {
  filename_ = filename;
  date_str_ = date;
  page_length_ = page_length;
  cex_flag_ = cex_flag;
}

void ListingGenerator::SetStartAddress(uint32_t addr) {
  start_address_ = addr;
}

void ListingGenerator::AddPageBreak() {
  pending_page_break_ = true;
}

std::vector<std::string> ListingGenerator::FormatLeftColumns(
    uint32_t addr, const std::vector<EmittedItem>& emitted, bool offset_mode, bool show_equal,
    int32_t equ_val, bool has_equ) const {
  std::vector<std::string> cols;
  std::string current_line;

  // Format initial address column (10 chars)
  char buf[32];
  if (offset_mode || show_equal) {
    std::snprintf(buf, sizeof(buf), "%08X= ", addr);
  } else {
    std::snprintf(buf, sizeof(buf), "%08X  ", addr);
  }
  current_line = buf;

  // EQU / SET direct value display
  if (has_equ) {
    std::snprintf(buf, sizeof(buf), "=%08X ", static_cast<uint32_t>(equ_val));
    current_line += buf;
    current_line.resize(32, ' ');
    cols.push_back(current_line);
    return cols;
  }

  for (const auto& item : emitted) {
    std::string item_str;
    if (item.size == 1) {
      std::snprintf(buf, sizeof(buf), "%02X ", item.value & 0xFF);
    } else if (item.size == 2) {
      std::snprintf(buf, sizeof(buf), "%04X ", item.value & 0xFFFF);
    } else {
      std::snprintf(buf, sizeof(buf), "%08X ", item.value);
    }
    item_str = buf;

    // Check if adding this item exceeds the 31 character boundary of left col
    if (current_line.length() + item.size > 31) {
      if (!cex_flag_) {
        // Truncate with "..."
        size_t overwrite_pos = (item.size == 2) ? 26 : 28;
        if (current_line.length() > overwrite_pos) {
          current_line = current_line.substr(0, overwrite_pos);
        }
        current_line.resize(overwrite_pos, ' ');
        current_line += "...";
        current_line.resize(32, ' ');
        cols.push_back(current_line);
        return cols;
      } else {
        // Flush current line and start continuation line with 10 spaces
        current_line.resize(32, ' ');
        cols.push_back(current_line);
        current_line = "          ";
      }
    }

    current_line += item_str;
  }

  current_line.resize(32, ' ');
  cols.push_back(current_line);
  return cols;
}

std::string ListingGenerator::ExpandTabs(const std::string& text) const {
  std::string result;
  int j = 0;  // output column index
  for (char c : text) {
    if (c == '\t') {
      int t = 1;
      if (j < 12) {
        t = 12 - j;
      } else if (j < 20) {
        t = 20 - j;
      } else if (j < 44) {
        t = 44 - j;
      } else {
        t = 8 - (j % 8);
      }
      result.append(t, ' ');
      j += t;
    } else {
      result.push_back(c);
      j++;
    }
  }
  return result;
}

void ListingGenerator::AddLine(uint32_t addr, const std::vector<EmittedItem>& emitted,
                               const std::string& src, int /*src_line*/, const std::string& ident,
                               bool offset_mode, bool show_equal, int32_t equ_val, bool has_equ) {
  // Check if we need to inject a page break
  bool is_first_page_break = pending_page_break_;
  if (pending_page_break_) {
    ListLine pb_line;
    pb_line.is_page_break = true;
    pb_line.is_error = false;
    pb_line.is_warning = false;
    pb_line.is_continuation = false;
    lines_.push_back(pb_line);
    pending_page_break_ = false;
  }

  std::vector<std::string> left_cols =
      FormatLeftColumns(addr, emitted, offset_mode, show_equal, equ_val, has_equ);

  // We save the main line content and continuation content.
  for (size_t idx = 0; idx < left_cols.size(); idx++) {
    ListLine newline;
    newline.is_page_break = (idx == 0 && is_first_page_break);
    newline.is_error = false;
    newline.is_warning = false;
    newline.is_continuation = (idx > 0);
    newline.left_column = left_cols[idx];

    if (idx == 0) {
      // Main line (includes right column: line number + ident + source line)
      std::string expanded_src = ExpandTabs(src);
      if (expanded_src.empty() || expanded_src.back() != '\n') {
        expanded_src += "\n";
      }

      std::string right_part = "  " + expanded_src;
      if (!ident.empty()) {
        right_part = ident + " " + expanded_src;
      }
      newline.right_column = right_part;
    } else {
      // Continuation line (only left column + newline)
      newline.right_column = "\n";
    }
    lines_.push_back(newline);
  }
}

void ListingGenerator::AddErrorLine(const std::string& message, bool is_warning) {
  ListLine err_line;
  err_line.is_page_break = false;
  err_line.is_error = true;
  err_line.is_warning = is_warning;
  err_line.is_continuation = false;
  err_line.right_column = message;
  lines_.push_back(err_line);
}

std::string ListingGenerator::Generate(int error_count, int warning_count,
                                       const std::string& symbol_table_cre) const {
  std::ostringstream out;

  // Header 1: Starting Address
  char start_buf[64];
  std::snprintf(start_buf, sizeof(start_buf), "%08X Starting Address\n", start_address_);
  out << start_buf;

  // Header 2: Assembler info & Created On date
  out << "Assembler used: EASy68K\n";
  out << "Created On: " << date_str_ << "\n\n";

  // Running lineNumL68 (starts at 1)
  int line_num_l68 = 1;

  // Pagination counters
  int page_num = 1;
  int lines_on_page = 0;

  auto print_page_header = [&](int pnum) {
    char header_buf[256];
    std::snprintf(header_buf, sizeof(header_buf),
                  "EASy68K 68000 Assembler   %-20.20s   %-20.20s   Page %d\n", filename_.c_str(),
                  date_str_.c_str(), pnum);
    out << header_buf;
    out << "================================================================================\n\n";
    lines_on_page = 2;  // header + separator + blank line
  };

  // Print first page header
  print_page_header(page_num);

  for (auto& line : lines_) {
    // Handle explicit PAGE break or page limit reached
    if (line.is_page_break || (page_length_ > 0 && lines_on_page >= page_length_)) {
      out << "\n<------------------------------ PAGE ------------------------------>\n\n";
      page_num++;
      print_page_header(page_num);
    }

    if (line.is_error) {
      // For error lines, display the CURRENT lineNumL68 (without incrementing it)
      out << "Line " << line_num_l68 << (line.is_warning ? " WARNING: " : " ERROR: ")
          << line.right_column << "\n";
    } else if (line.is_continuation) {
      // For continuation lines, only print the left column and the newline
      out << line.left_column << line.right_column;
    } else {
      // For main source lines, format the line number right-aligned to a 6-character field
      char num_buf[16];
      std::snprintf(num_buf, sizeof(num_buf), "%6d", line_num_l68);
      out << line.left_column << num_buf << line.right_column;
      line_num_l68++;
    }

    lines_on_page++;
  }

  // Final summary block
  out << "\n";
  if (error_count > 0) {
    char err_buf[64];
    std::snprintf(err_buf, sizeof(err_buf), "%d error%s detected\n", error_count,
                  (error_count > 1) ? "s" : "");
    out << err_buf;
  } else {
    out << "No errors detected\n";
  }

  if (warning_count > 0) {
    char warn_buf[64];
    std::snprintf(warn_buf, sizeof(warn_buf), "%d warning%s generated\n", warning_count,
                  (warning_count > 1) ? "s" : "");
    out << warn_buf;
  } else {
    out << "No warnings generated\n";
  }

  // OPT CRE Symbol Table display
  if (!symbol_table_cre.empty()) {
    out << symbol_table_cre;
  }

  return out.str();
}

}  // namespace easym68k::asm_
