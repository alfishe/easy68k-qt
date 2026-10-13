// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Stand-ins for the Edit68K forms referenced by the original assembler core
// (Options, Main, TTextStuff, AssemblerBox, error list items). Reference
// oracle build only (tools/reference).

#ifndef EASY68K_REFERENCE_GUI_STUBS_H_
#define EASY68K_REFERENCE_GUI_STUBS_H_

#include <cstdio>
#include <vector>

#include "vcl.h"

struct TCheckBoxStub {
  bool Checked = false;
};

struct TOptionsStub {
  TCheckBoxStub gen_list, gen_srec, constants_ex, bitfield, cross_ref, mac_ex, struc_ex, warnings;
  TCheckBoxStub* chkGenList = &gen_list;
  TCheckBoxStub* chkGenSRec = &gen_srec;
  TCheckBoxStub* chkConstantsEx = &constants_ex;
  TCheckBoxStub* chkBitfield = &bitfield;
  TCheckBoxStub* chkCrossRef = &cross_ref;
  TCheckBoxStub* chkMacEx = &mac_ex;
  TCheckBoxStub* chkStrucEx = &struc_ex;
  TCheckBoxStub* chkShowWarnings = &warnings;
  bool bSave = false;
};
extern TOptionsStub* Options;

// Error list item: Caption = line number, SubItems = message (and file).
struct TStringsStub {
  std::vector<AnsiString> items;
  void Add(const AnsiString& s) { items.push_back(s); }
  void Clear() { items.clear(); }
};
struct TListItem {
  AnsiString Caption;
  TStringsStub storage;
  TStringsStub* SubItems = &storage;
};
struct TListItemsStub {
  std::vector<TListItem*> items;
  TListItem* Add() {
    items.push_back(new TListItem);
    return items.back();
  }
  void Clear() {
    for (TListItem* i : items)
      delete i;
    items.clear();
  }
};
struct TMessagesStub {
  int Height = 0;
  bool Enabled = false;
  TListItemsStub list;
  TListItemsStub* Items = &list;
};
struct TProjectStub {
  AnsiString CurrentFile;
  bool Modified = false;
  bool IsSource = true;
  int TabType = 1;  // tabTypes: 0 = Assembly, 1 = Fixed (editor default)
  int TabSize = 4;  // editorOptions.dfm FixedTabSize default
};
struct TTextStuff {
  TMessagesStub messages;
  TMessagesStub* Messages = &messages;
  TProjectStub Project;
  void BringToFront() {}
};
extern TTextStuff* TextStuff;

struct TMainStub {
  TTextStuff* ActiveMDIChild = nullptr;
  int MDIChildCount = 0;
  TTextStuff* MDIChildren[1] = {nullptr};
  void OpenFile(const AnsiString&) {}
};
extern TMainStub* Main;

struct TLabelStub {
  AnsiString Caption;
};
struct TButtonStub {
  bool Enabled = false;
};
struct TAssemblerBoxStub {
  TLabelStub status, status2;
  TButtonStub execute;
  TLabelStub* lblStatus = &status;
  TLabelStub* lblStatus2 = &status2;
  TButtonStub* cmdExecute = &execute;
};
extern TAssemblerBoxStub* AssemblerBox;

#endif  // EASY68K_REFERENCE_GUI_STUBS_H_
