// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// AsmCore — the original EASy68K assembler (Edit68K: ASSEMBLE.CPP,
// DIRECTIV.CPP, MACRO.CPP, STRUCTURED.CPP, LISTING.CPP, SYMBOL.CPP,
// EVAL.CPP, OPPARSE.CPP, OBJECT.CPP, ERROR.CPP, MOVEM.CPP, INSTLOOK.CPP,
// CODEGEN.CPP by Paul McKee, Charles Kelly and Tim Larson) ported to portable
// C++. The routines keep the original structure and names so they can be
// compared with the original line by line; the original's global variables
// are members of AsmCore, its files are in-memory MemFiles, and the Windows
// GUI calls are replaced by the options and callbacks below.
//
// The instruction flavors and building routines (INSTTABL.CPP, BUILD.CPP)
// are shared with easym68k/asm/instruction_set.h.

#ifndef EASYM68K_SRC_ASM_CORE_ASM_CORE_H_
#define EASYM68K_SRC_ASM_CORE_ASM_CORE_H_

#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "easym68k/asm/instruction_set.h"
#include "easym68k/asm/original_status.h"

namespace easym68k::asm_::core {

// ---------------------------------------------------------------------------
// asm.h definitions
// ---------------------------------------------------------------------------

#define isTerm(c) (c == ',' || c == '/' || c == '-' || isspace(c) || !c || c == '{')
#define isRegNum(c) ((c >= '0') && (c <= '7'))

inline constexpr char TITLE[] = "EASy68K Editor/Assembler v5.16.01";

using namespace ::easym68k::asm_::status;  // NOLINT: original status names below

inline constexpr int OK = kOk;
inline constexpr int SEVERE = kSevere;
inline constexpr int SYNTAX = kSyntax;
inline constexpr int INV_OPCODE = kInvOpcode;
inline constexpr int INV_ADDR_MODE = kInvAddrMode;
inline constexpr int LABEL_REQUIRED = kLabelRequired;
inline constexpr int NO_ENDM = kNoEndm;
inline constexpr int TOO_MANY_ARGS = kTooManyArgs;
inline constexpr int INVALID_ARG = kInvalidArg;
inline constexpr int COMMA_EXPECTED = kCommaExpected;
inline constexpr int PHASE_ERROR = kPhaseError;
inline constexpr int FILE_ERROR = kFileError;
inline constexpr int MACRO_NEST = kMacroNest;
inline constexpr int NO_IF = kNoIf;
inline constexpr int NO_WHILE = kNoWhile;
inline constexpr int NO_REPEAT = kNoRepeat;
inline constexpr int NO_FOR = kNoFor;
inline constexpr int ENDI_EXPECTED = kEndiExpected;
inline constexpr int ENDW_EXPECTED = kEndwExpected;
inline constexpr int ENDF_EXPECTED = kEndfExpected;
inline constexpr int REPEAT_EXPECTED = kRepeatExpected;
inline constexpr int LABEL_ERROR = kLabelError;
inline constexpr int NO_DBLOOP = kNoDbloop;
inline constexpr int DBLOOP_EXPECTED = kDbloopExpected;
inline constexpr int BAD_BITFIELD = kBadBitfield;
inline constexpr int ILLEGAL_SYMBOL = kIllegalSymbol;
inline constexpr int EXCEPTION = 0x999;
inline constexpr int ERRORN = kError;
inline constexpr int UNDEFINED = kUndefined;
inline constexpr int DIV_BY_ZERO = kDivByZero;
inline constexpr int MULTIPLE_DEFS = kMultipleDefs;
inline constexpr int REG_MULT_DEFS = kRegMultDefs;
inline constexpr int REG_LIST_UNDEF = kRegListUndef;
inline constexpr int INV_FORWARD_REF = kInvForwardRef;
inline constexpr int INV_LENGTH = kInvLength;
inline constexpr int MINOR = kMinor;
inline constexpr int INV_SIZE_CODE = kInvSizeCode;
inline constexpr int INV_QUICK_CONST = kInvQuickConst;
inline constexpr int INV_MOVE_QUICK_CONST = kInvMoveQuickConst;
inline constexpr int INV_VECTOR_NUM = kInvVectorNum;
inline constexpr int INV_BRANCH_DISP = kInvBranchDisp;
inline constexpr int INV_DISP = kInvDisp;
inline constexpr int INV_ABS_ADDRESS = kInvAbsAddress;
inline constexpr int INV_8_BIT_DATA = kInv8BitData;
inline constexpr int INV_16_BIT_DATA = kInv16BitData;
inline constexpr int NOT_REG_LIST = kNotRegList;
inline constexpr int REG_LIST_SPEC = kRegListSpec;
inline constexpr int INV_SHIFT_COUNT = kInvShiftCount;
inline constexpr int INV_OPERATOR = kInvOperator;
inline constexpr int FAIL_ERROR = kFailError;
inline constexpr int WARNING = kWarning;
inline constexpr int ASCII_TOO_BIG = kAsciiTooBig;
inline constexpr int NUMBER_TOO_BIG = kNumberTooBig;
inline constexpr int INCOMPLETE = kIncomplete;
inline constexpr int FORCING_SHORT = kForcingShort;
inline constexpr int ODD_ADDRESS = kOddAddress;
inline constexpr int END_MISSING = kEndMissing;
inline constexpr int ADDRESS_MISSING = kAddressMissing;
inline constexpr int THEN_EXPECTED = kThenExpected;
inline constexpr int DO_EXPECTED = kDoExpected;
inline constexpr int FORWARD_REF = kForwardRef;
inline constexpr int LABEL_TOO_LONG = kLabelTooLong;
inline constexpr int SEVERITY = kSeverity;

#define NEWERROR(var, code) var = (((code) & SEVERITY) > var) ? (code) : var

inline constexpr int SIGCHARS = 33;
inline constexpr int MAX_ARGS = 36;
inline constexpr int ARG_SIZE = 256;

using opDescriptor = ::easym68k::asm_::OpDescriptor;

struct symbolDef {
  int value = 0;
  symbolDef* next = nullptr;
  char flags = 0;
  char name[SIGCHARS + 1] = {};
};

inline constexpr int BACKREF = 0x01;
inline constexpr int REDEFINABLE = 0x02;
inline constexpr int REG_LIST_SYM = 0x04;
inline constexpr int MACRO_SYM = 0x08;
inline constexpr int DS_SYM = 0x10;

inline constexpr int NORMAL = 0;
inline constexpr int MILD_ERROR = 1;
inline constexpr int CRITICAL = 2;

enum tabTypes { Assembly, Fixed };

inline constexpr int TAB1 = 12;
inline constexpr int TAB2 = 20;
inline constexpr int TAB3 = 44;
inline constexpr int MACRO_NEST_LIMIT = 256;
inline constexpr int MEM_SIZE = 0x00FFFFFF;  // upper limit of 68000 memory
inline constexpr char NEW_PAGE_MARKER[] =
    "<------------------------------ PAGE ------------------------------>";

// ---------------------------------------------------------------------------
// Borland run-time library replacements.
// ---------------------------------------------------------------------------
int stricmp(const char* a, const char* b);
int strnicmp(const char* a, const char* b, size_t n);
inline int strcmpi(const char* a, const char* b) {
  return stricmp(a, b);
}

// Minimal AnsiString (VCL) with the operations used by STRUCTURED.CPP.
class AnsiString {
 public:
  AnsiString() = default;
  AnsiString(const char* s) : s_(s ? s : "") {}  // NOLINT: implicit like VCL
  AnsiString(const std::string& s) : s_(s) {}    // NOLINT
  AnsiString(char c) : s_(1, c) {}               // NOLINT
  AnsiString& operator=(const char* s) {
    s_ = s ? s : "";
    return *this;
  }
  AnsiString& operator+=(const AnsiString& o) {
    s_ += o.s_;
    return *this;
  }
  friend AnsiString operator+(const AnsiString& a, const AnsiString& b) {
    return AnsiString(a.s_ + b.s_);
  }
  friend AnsiString operator+(const char* a, const AnsiString& b) {
    return AnsiString(std::string(a) + b.s_);
  }
  friend AnsiString operator+(const AnsiString& a, const char* b) {
    return AnsiString(a.s_ + b);
  }
  bool operator==(const char* o) const { return s_ == o; }
  bool operator==(const AnsiString& o) const { return s_ == o.s_; }
  bool operator!=(const char* o) const { return s_ != o; }
  // Borland's c_str() returns a modifiable buffer.
  char* c_str() { return &s_[0]; }
  const char* c_str() const { return s_.c_str(); }
  int Length() const { return static_cast<int>(s_.size()); }
  AnsiString UpperCase() const;
  static AnsiString IntToHex(unsigned int value, int digits);
  const std::string& str() const { return s_; }

 private:
  std::string s_;
};
using String = AnsiString;

// ---------------------------------------------------------------------------
// In-memory replacement for the original's FILE streams.
// ---------------------------------------------------------------------------
class MemFile {
 public:
  MemFile() = default;
  explicit MemFile(std::string data) : data_(std::move(data)) {}
  std::string data_;
  size_t pos_ = 0;
  bool eof_ = false;
};

char* fgets(char* s, int n, MemFile* f);
int fprintf(MemFile* f, const char* format, ...);
int fputs(const char* s, MemFile* f);
int putc(int c, MemFile* f);
long ftell(MemFile* f);
int fseek(MemFile* f, long offset, int origin);
void rewind(MemFile* f);
int feof(MemFile* f);
int ferror(MemFile* f);
size_t fread(void* ptr, size_t size, size_t count, MemFile* f);

// ---------------------------------------------------------------------------
// Options (the original's Options dialog and editor settings).
// ---------------------------------------------------------------------------
struct CoreOptions {
  bool generate_list = true;  // Options->chkGenList
  bool generate_srec = true;  // objFlag
  bool bitfield = true;       // BITflag (Assemble bit field instructions)
  bool cre = true;            // CREflag (symbol table in listing)
  bool cex = false;           // CEXflag
  bool mex = false;           // MEXflag
  bool sex = false;           // SEXflag
  bool warnings = true;       // WARflag (show warnings)
  tabTypes tab_type = Fixed;  // Project.TabType
  int tab_size = 4;           // Project.TabSize
  std::string created_on;     // "Created On:" text of the listing
  std::string file_name;      // source file name (S0 record, messages)
};

// One diagnostic as printed by printError(): line number and status.
struct CoreMessage {
  int line;
  int code;
  std::string text;  // full text, e.g. "ERROR: Invalid syntax"
  std::string file;  // include file name, empty for the main source
};

// Reads an include/incbin file; returns false if it cannot be read.
using CoreFileReader = std::function<bool(const std::string& name, std::string* contents)>;

class AsmCore {
 public:
  AsmCore(CoreOptions options, CoreFileReader reader);
  ~AsmCore();

  // Assembles `source` (two passes, like assembleFile()/processFile()).
  void AssembleSource(const std::string& source);

  std::string Listing() const { return listFile ? listFile->data_ : std::string(); }
  std::string SRecords() const { return objFile ? objFile->data_ : std::string(); }
  const std::vector<CoreMessage>& Messages() const { return messages_; }
  int ErrorCount() const { return errorCount; }
  int WarningCount() const { return warningCount; }
  unsigned int StartAddress() const { return startAddress; }
  // Object code written in pass 2, by address.
  const std::map<uint32_t, uint8_t>& Image() const { return image_; }

  // Symbols (name, value, flags) in the order of the original's table.
  struct Symbol {
    std::string name;
    int value;
    int flags;
  };
  std::vector<Symbol> Symbols() const;

 public:
  using DirectiveFn = int (AsmCore::*)(int, char*, char*, int*);
  // Instruction table entry (asm.h instruction).
  struct instruction {
    const char* mnemonic;
    const ::easym68k::asm_::Instruction* flavors;  // parseFlag instructions
    bool parseFlag;
    DirectiveFn exec;  // routine called if parseFlag is false
  };
  static const std::vector<instruction>& InstTable();

 private:
  CoreOptions options_;
  CoreFileReader reader_;
  std::vector<CoreMessage> messages_;
  std::map<uint32_t, uint8_t> image_;

  // ===== GLOBALS.CPP =====
  int loc = 0;
  int locOffset = 0;
  int sectionLoc[16] = {};
  int sectI = 0;
  bool offsetMode = false;
  bool showEqual = false;
  char pass = 0;
  bool pass2 = false;
  bool endFlag = false;
  int labelNum = 0;
  char buffer[1024] = {};
  char numBuf[20] = {};
  int errorCount = 0, warningCount = 0;
  char empty[1] = "";
  unsigned int startAddress = 0;
  char globalLabel[SIGCHARS + 1] = {};
  int includeNestLevel = 0;
  char includeFile[256] = {};
  bool includedFileError = false;
  MemFile* inFile = nullptr;
  MemFile* listFile = nullptr;
  MemFile* objFile = nullptr;
  MemFile* tmpFile = nullptr;
  char line[512] = {};
  int lineNum = 0;
  int lineNumL68 = 0;
  char* listPtr = nullptr;
  bool continuation = false;
  bool listFlag = true;
  bool objFlag = true;
  bool CEXflag = false;
  bool BITflag = true;
  bool CREflag = true;
  bool MEXflag = false;
  bool SEXflag = false;
  bool WARflag = true;
  unsigned int stcLabelI = 0, stcLabelW = 0, stcLabelR = 0, stcLabelF = 0, stcLabelD = 0;
  bool mapROM = false, mapRead = false, mapProtected = false, mapInvalid = false;
  int mapROMStart = 0, mapROMEnd = 0;
  int mapReadStart = 0, mapReadEnd = 0;
  int mapProtectedStart = 0, mapProtectedEnd = 0;
  int mapInvalidStart = 0, mapInvalidEnd = 0;

  // ===== ASSEMBLE.CPP =====
  bool skipList = false;
  bool skipCond = false;
  bool printCond = false;
  bool skipCreateCode = false;
  static constexpr int MAXT = 128;
  static constexpr int MAX_SIZE = 512;
  char* token[MAXT] = {};
  char tokens[MAX_SIZE] = {};
  char* tokenEnd[MAXT] = {};
  int nestLevel = 0;

  // ===== MACRO.CPP =====
  int macroFP = 0;
  static constexpr int MAC_SIZE = 512;
  int macroNestLevel = 0;
  char lineIdent[MACRO_NEST_LIMIT + 2] = {};
  bool noENDM = false;

  // ===== STRUCTURED.CPP =====
  AnsiString stcLabel;
  std::stack<int, std::vector<int>> stcStack;
  std::stack<char, std::vector<char>> dbStack;
  std::stack<String, std::vector<String>> forStack;

  // ===== LISTING.CPP =====
  char listData[64] = {};
  bool createdL68 = false;

  // ===== SYMBOL.CPP =====
  static constexpr int MAXHASH = 26;
  symbolDef* htable[MAXHASH + 1] = {};
  bool symbolInit = false;

  // ===== OBJECT.CPP =====
  char sRecord[80] = {};
  char* objPtr = nullptr;
  char byteCount = 0, checksum = 0;
  bool lineFlag = false;
  int objAddr = 0;

  // Files owned by the core (main source, temp, listing, object, includes).
  std::vector<std::unique_ptr<MemFile>> files_;
  MemFile* NewFile(std::string data = std::string());
  // Opens an INCLUDE/INCBIN file through the reader (fopen() in the
  // original, on a case-insensitive file system); nullptr if it cannot be
  // read. `name` is the capitalized file name the original opens; `operand`
  // points to the operand in the capitalized line, so the name can also be
  // tried as written in the source.
  MemFile* OpenFile(const char* name, const char* operand);
  const char* curLine_ = nullptr;     // line being assembled
  const char* curCapLine_ = nullptr;  // its capitalized copy
  void runFlavor(const Flavor* flavorPtr, int mask, int size, opDescriptor* source,
                 opDescriptor* dest, int* errorPtr);

  // ===== Original routines =====
  // ASSEMBLE.CPP
  int processFile();
  int assemble(char* line, int* errorPtr);
  int createCode(char* capLine, int* errorPtr);
  char* fieldParse(char* p, opDescriptor* d, int* errorPtr);
  int strcap(char* d, char* s);
  char* skipSpace(char* p);
  int tokenize(char* instr, const char* delim, char* token[], char* tokens);
  // DIRECTIV.CPP
  int org(int size, char* label, char* op, int* errorPtr);
  int section(int size, char* label, char* op, int* errorPtr);
  int offset(int size, char* label, char* op, int* errorPtr);
  int funct_end(int size, char* label, char* op, int* errorPtr);
  int equ(int size, char* label, char* op, int* errorPtr);
  int set(int size, char* label, char* op, int* errorPtr);
  int dc(int size, char* label, char* op, int* errorPtr);
  char* collect(char* s, char* d);
  int dcb(int size, char* label, char* op, int* errorPtr);
  int ds(int size, char* label, char* op, int* errorPtr);
  int opt(int size, char* label, char* op, int* errorPtr);
  int include(int size, char* label, char* fileName, int* errorPtr);
  int incbin(int size, char* label, char* fileName, int* errorPtr);
  int failError(int size, char* label, char* message, int* errorPtr);
  int listOn(int size, char* label, char* message, int* errorPtr);
  int listOff(int size, char* label, char* message, int* errorPtr);
  int memory(int size, char* label, char* op, int* errorPtr);
  int simhalt(int size, char* label, char* op, int* errorPtr);
  int page(int size, char* label, char* op, int* errorPtr);
  // MACRO.CPP
  int macro(int size, char* label, char* op, int* errorPtr);
  int asmMacro(int size, char* label, char* arg, int* errorPtr);
  // STRUCTURED.CPP
  AnsiString getBcc(AnsiString cc, int mode, int orFlag);
  void outCmpBcc(char* token[], char* last, AnsiString label, int& error);
  int asmStructure(int size, char* label, char* arg, int* errorPtr);
  void assembleStc(char* line);
  void assembleStc(const char* line) { assembleStc(const_cast<char*>(line)); }
  // LISTING.CPP
  int initList();
  int listLine(const char text[], const char lineIdent[] = "");
  int listLoc();
  int listCond(bool cond);
  int listError(const char* lineNum, const char* errMsg);
  int listText(const char* text);
  int listObj(int data, int size);
  int finishList();
  // SYMBOL.CPP
  void clearSymbols();
  symbolDef* lookup(char* sym, int create, int* errorPtr);
  int optCRE();
  int hash(char* symbol);
  symbolDef* define(char* sym, int value, bool pass2, bool check, int* errorPtr);
  // EVAL.CPP
  char* eval(char* p, int* valuePtr, bool* refPtr, int* errorPtr);
  char* evalNumber(char* p, int* numberPtr, bool* refPtr, int* errorPtr);
  static int precedence(char op);
  static int doOp(int val1, int val2, char op, int* result);
  // OPPARSE.CPP
  char* opParse(char* p, opDescriptor* d, int* errorPtr);
  // OBJECT.CPP
  int initObj();
  int outputObj(int newAddr, int data, int size);
  int writeObj();
  int writeMap(const char* name, int start, int end);
  int finishObj();
  // ERROR.CPP
  int printError(MemFile* outFile, int errorCode, int lineNum);
  // MOVEM.CPP
  int movem(int size, char* label, char* op, int* errorPtr);
  int reg(int size, char* label, char* op, int* errorPtr);
  char* evalList(char* p, unsigned short* listPtr, int* errorPtr);
  // INSTLOOK.CPP / INSTTABL.CPP
  char* instLookup(char* p, const instruction** instPtrPtr, char* sizePtr, int* errorPtr);
  // CODEGEN.CPP
  int output(int data, int size);
  int effAddr(opDescriptor* operand);
  int extWords(opDescriptor* op, int size, int* errorPtr);

};

}  // namespace easym68k::asm_::core

#endif  // EASYM68K_SRC_ASM_CORE_ASM_CORE_H_
