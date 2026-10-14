// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from Edit68K/ERROR.CPP of the original EASy68K (Paul McKee, Charles
// Kelly, Tim Larson). The functions follow the original line by line; the
// original's global state lives in AsmCore (see asm_core.h).

#include "asm_core.h"

namespace easym68k::asm_::core {

/***********************************************************************
 *
 *		ERRORN.C
 *		Error message printer for 68000 Assembler
 *
 *    Function: printError()
 *		Prints an appropriate message to the specified output
 *		file according to the error code supplied. If the
 *		errorCode is OK, no message is printed; otherwise an
 *		WARNING or ERRORN message is produced. The line number
 *		will be included in the message unless lineNum = -1.
 *
 *	 Usage:	printError(outFile, errorCode, lineNum)
 *		FILE *outFile;
 *		int errorCode, lineNum;
 *
 *      Author: Paul McKee
 *		ECE492    North Carolina State University
 *
 *        Date:	12/12/86
 *
 *    Modified: Charles Kelly
 *              Monroe County Community College
 *              http://www.monroeccc.edu/ckelly
 *
 *              Mar 15, 2002 by Tim Larson
 *              - added code to dump error messages into active forms listview
 *              - added numbers to each error message
 ************************************************************************/




int AsmCore::printError(MemFile *outFile, int errorCode, int lineNum)
{
  if (errorCode == OK)          // if no errors detected
    return NORMAL;
  // if there are only warnings and warnings are disabled
  if (errorCode < MINOR && errorCode > WARNING && WARflag == false)
    return NORMAL;

  if (lineNumL68 >= 0)
    sprintf(numBuf, "Line %d ", lineNumL68);
  else
    numBuf[0] = '\0';

  // ---------- display warning messages ----------
  switch (errorCode) {
    case NUMBER_TOO_BIG:
      sprintf(buffer, "WARNING: Numeric constant exceeds 32 bits\n");
      break;
    case ASCII_TOO_BIG:
      sprintf(buffer, "WARNING: ASCII constant exceeds 4 characters\n");
      break;
    case INCOMPLETE:
      sprintf(buffer, "WARNING: Evaluation of expression could not be completed\n");
      break;
    case FORCING_SHORT:
      sprintf(buffer, "WARNING: Forcing SHORT addressing disables range checking of extension word\n");
      break;
    case ODD_ADDRESS:
      sprintf(buffer, "WARNING: Origin value is odd (Location counter set to next highest address)\n");
      break;
    case END_MISSING:
      sprintf(buffer, "WARNING: END directive missing, starting address not set\n");
      break;
    case ADDRESS_MISSING:
      sprintf(buffer, "WARNING: Address expected\n");
      break;
    case THEN_EXPECTED:
      sprintf(buffer, "WARNING: THEN expected\n");
      break;
    case DO_EXPECTED:
      sprintf(buffer, "WARNING: DO expected\n");
      break;
    case FORWARD_REF:
      sprintf(buffer, "WARNING: Forward reference may cause 'Symbol value differs...' error\n");
      break;
    case LABEL_TOO_LONG:
      sprintf(buffer, "WARNING: Label too long\n");
      break;
    default :
      if (errorCode < MINOR && errorCode > WARNING)
        sprintf(buffer, "WARNING: No message defined\n");
  }

  // ---------- display error messages ----------
  switch (errorCode) {
    case SYNTAX:
      sprintf(buffer, "ERROR: Invalid syntax\n");
      break;
    case UNDEFINED:
      sprintf(buffer, "ERROR: Undefined symbol\n");
      break;
    case DIV_BY_ZERO:
      sprintf(buffer, "ERROR: Division by zero attempted\n");
      break;
    case INV_OPCODE:
      sprintf(buffer, "ERROR: Invalid opcode\n");
      break;
    case INV_SIZE_CODE:
      sprintf(buffer, "ERROR: Invalid size code\n");
      break;
    case INV_ADDR_MODE:
      sprintf(buffer, "ERROR: Invalid addressing mode\n");
      break;
    case MULTIPLE_DEFS:
      sprintf(buffer, "ERROR: Symbol defined more than once\n");
      break;
    case NO_ENDM:
      sprintf(buffer, "ERROR: ENDM expected\n");
      break;
    case TOO_MANY_ARGS:
      sprintf(buffer, "ERROR: Too many arguments\n");
      break;
    case INVALID_ARG:
      sprintf(buffer, "ERROR: Invalid argument\n");
      break;
    case PHASE_ERROR:
      sprintf(buffer, "ERROR: Symbol value differs between first and second pass\n");
      break;
    case INV_QUICK_CONST:
      sprintf(buffer, "ERROR: Quick immediate data range must be 1 to 8\n");
      break;
    case INV_MOVE_QUICK_CONST:
      sprintf(buffer, "ERROR: Quick immediate data range exceeds 1 byte\n");
      break;
    case INV_VECTOR_NUM:
      sprintf(buffer, "ERROR: Invalid vector number\n");
      break;
    case INV_BRANCH_DISP:
      sprintf(buffer, "ERROR: Branch instruction displacement is out of range or invalid\n");
      break;
    case LABEL_REQUIRED:
      sprintf(buffer, "ERROR: Label required with this directive\n");
      break;
    case INV_DISP:
      sprintf(buffer, "ERROR: Displacement out of range\n");
      break;
    case INV_ABS_ADDRESS:
      sprintf(buffer, "ERROR: Absolute address exceeds 16 bits\n");
      break;
    case INV_8_BIT_DATA:
      sprintf(buffer, "ERROR: Immediate data exceeds 8 bits\n");
      break;
    case INV_16_BIT_DATA:
      sprintf(buffer, "ERROR: Immediate data exceeds 16 bits\n");
      break;
    case NOT_REG_LIST:
      sprintf(buffer, "ERROR: Symbol is not a register list symbol\n");
      break;
    case REG_LIST_SPEC:
      sprintf(buffer, "ERROR: Register list symbol used in an expression\n");
      break;
    case REG_LIST_UNDEF:
      sprintf(buffer, "ERROR: Register list symbol not previously defined\n");
      break;
    case INV_SHIFT_COUNT:
      sprintf(buffer, "ERROR: Invalid constant shift count\n");
      break;
    case INV_OPERATOR:
      sprintf(buffer, "ERROR: Invalid operator\n");
      break;
    case INV_FORWARD_REF:
      sprintf(buffer, "ERROR: Forward references not allowed with this directive\n");
      break;
    case INV_LENGTH:
      sprintf(buffer, "ERROR: Invalid block length\n");
      break;
    case COMMA_EXPECTED:
      sprintf(buffer, "ERROR: Comma expected\n");
      break;
    case FAIL_ERROR:    // user defined error
      //the error message has already been written to buffer
      break;
    case EXCEPTION:     // an exception occurred
      //the error message has already been written to buffer
      break;
    case FILE_ERROR:
      sprintf(buffer, "ERROR: Unable to access specified file\n");
      break;
    case MACRO_NEST:    // possible recursive macro call
      sprintf(buffer, "ERROR: Nested Macro calls are too many levels deep\n");
      break;
    case NO_IF:
      sprintf(buffer, "ERROR: No matching IF statement was found\n");
      break;
    case NO_WHILE:
      sprintf(buffer, "ERROR: No matching WHILE statement was found\n");
      break;
    case NO_REPEAT:
      sprintf(buffer, "ERROR: No matching REPEAT statement was found\n");
      break;
    case NO_DBLOOP:
      sprintf(buffer, "ERROR: No matching DBLOOP statement was found\n");
      break;
    case NO_FOR:
      sprintf(buffer, "ERROR: No matching FOR statement was found\n");
      break;
    case ENDI_EXPECTED:
      sprintf(buffer, "ERROR: IF without ENDI\n");
      break;
    case ENDW_EXPECTED:
      sprintf(buffer, "ERROR: WHILE without ENDW\n");
      break;
    case ENDF_EXPECTED:
      sprintf(buffer, "ERROR: FOR without ENDF\n");
      break;
    case REPEAT_EXPECTED:
      sprintf(buffer, "ERROR: UNTIL without REPEAT\n");
      break;
    case DBLOOP_EXPECTED:
      sprintf(buffer, "ERROR: UNLESS without DBLOOP\n");
      break;
    case LABEL_ERROR:
      sprintf(buffer, "ERROR: Label is not allowed\n");
      break;
    case BAD_BITFIELD:
      sprintf(buffer, "ERROR: Invalid bitfield\n");
      break;
    case ILLEGAL_SYMBOL:
      sprintf(buffer, "ERROR: Illegal symbol\n");
      break;
    default :
      if (errorCode > MINOR)
        sprintf(buffer, "ERROR: No message defined\n");
  } // end switch

  if (outFile)
    listError(numBuf, buffer);  // add error to listing file

  // display error messages in edit window: the original adds the line
  // number (source line), message and file to the editor's message list
  // (opening an include file's window when needed); the port records them.
  std::string text = buffer;
  if (!text.empty() && text.back() == '\n')
    text.pop_back();  // trim off '\n'
  messages_.push_back({lineNum, errorCode, text, includeFile});

  return NORMAL;
}



}  // namespace easym68k::asm_::core
