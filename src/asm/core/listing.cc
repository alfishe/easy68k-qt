// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from Edit68K/LISTING.CPP of the original EASy68K (Paul McKee, Charles
// Kelly, Tim Larson). The functions follow the original line by line; the
// original's global state lives in AsmCore (see asm_core.h).

#include "asm_core.h"

namespace easym68k::asm_::core {

/***********************************************************************
 *
 *		LISTING.C
 *		Listing File Routines for 68000 Assembler
 *
 *    Function: initList()
 *		Opens the specified listing file for writing. If the
 *		file cannot be opened, then the routine prints a
 *		message and exits.
 *
 *		listLine(char text[])
 *		Writes the specified line to the listing file. If
 *		the line is not a continuation, then the routine 
 *		includes the source line as the last part of the
 *		listing line. If an error occurs during the writing, 
 *		the routine prints a message and exits.
 *
 *		listLoc()
 *		Starts the process of assembling a listing line by 
 *		printing the location counter value into listData and
 *		initializing listPtr.
 *
 *		listObj()
 *		Prints the data whose size and value are specified in
 *		the object field of the current listing line. Bytes are
 *		printed as two digits, words as four digits, and
 *		longwords as eight digits, and a space follows each
 *		group of digits. 
 *		     If the item to be printed will not fit in the
 *		object code field, one of two things will occur. If
 *		CEXflag is TRUE, then the current listing line will be
 *		output to the listing file and the data will be printed
 *		on a continuation line. Otherwise, elipses ("...") will
 *		be printed to indicate the omission of values from the
 *		listing, and the data will not be added to the file. 
 *
 *	 Usage: initList(name)
 *		char *name;
 *
 *		listLine(line)
 *
 *		listLoc()
 *
 *		listObj(data, size)
 *		int data, size;
 *
 *      Author: Paul McKee
 *		ECE492    North Carolina State University
 *
 *        Date:	12/13/86
 *
 *    Modified: Charles Kelly
 *              Monroe County Community College
 *              http://www.monroeccc.edu/ckelly
 *
 ************************************************************************/



/* Declarations of global variables */

// listData (the buffer in which listing lines are assembled; 49 bytes in
// the original), listPtr (manipulated by equ() and set() to put specially
// formatted information in the listing) and createdL68 are AsmCore members.

int AsmCore::initList()
{
  try {
    createdL68 = false;
    listFile = NewFile();

    // The original prints DateTimeToStr(Now()); the port uses the caller's
    // text (CoreOptions::created_on).
    const std::string& timeStr = options_.created_on;

    // reserve room for starting address
    fprintf(listFile, "00000000 Starting Address\n");

    fprintf(listFile, "Assembler used: %s\n", TITLE);
    fprintf(listFile, "Created On: %s\n\n", timeStr.c_str());

    createdL68 = true;
    return NORMAL;
  }
  catch( ... ) {
    snprintf(buffer, sizeof(buffer), "ERROR: An exception occurred in routine 'initList'. \n");
    printError(NULL, EXCEPTION, 0);
    return MILD_ERROR;
  }
}

int AsmCore::listLine(const char text[], const char lineIdent[])   // ck 4-2006 lineIdent[]
{
  // FixedTabSize->Value
  try {
    if (!createdL68)
      return NORMAL;
    fprintf(listFile, "%-32.32s", listData);
    if (!continuation) {
      // replace tab with spaces
      int i=0, j=0, k, t;
      while (text[i] && j < 255-8) {
	if (text[i] == '\t') {          // if tab
          if (options_.tab_type == Assembly) {
            if (j <= TAB1)
              t = TAB1 - j;
            else if (j <= TAB2)
              t = TAB2 - j;
            else
              t = TAB3 - j;
          } else {                      // else fixed tabs
            t = options_.tab_size - (j % options_.tab_size);

          }
          for (k=0; k<t; k++)       // replace with spaces
	    buffer[j++] = ' ';
        } else
          buffer[j++] = text[i];        // else, copy character
        i++;
      }
      if (j>0 && buffer[j-1] != '\n')   // if line does not end in '\n'
        buffer[j++] = '\n';             // add it
      buffer[j] = '\0';

      if (lineIdent[0])                 // if line identifier
        fprintf(listFile, "%6d%s %s", lineNumL68, lineIdent, buffer);
      else
        fprintf(listFile, "%6d  %s", lineNumL68, buffer);
    } else
      putc('\n', listFile);

    if (ferror(listFile)) {
      snprintf(buffer, sizeof(buffer), "Error writing to listing file\n");
      return MILD_ERROR;
    }
    lineNumL68++;
  }
  catch( ... ) {
    snprintf(buffer, sizeof(buffer), "ERROR: An exception occurred in routine 'listLine'. \n");
    printError(NULL, EXCEPTION, 0);
    return MILD_ERROR;
  }

  return NORMAL;
}


int AsmCore::listLoc()
{
  if (offsetMode || showEqual)
    snprintf(listData, sizeof(listData), "%08lX= ", loc);
  else
    snprintf(listData, sizeof(listData), "%08lX  ", loc);
  listPtr = listData + 10;

  return NORMAL;
}

// Lists the value of skipCond
// when skipCond is True the conditional test was False
int AsmCore::listCond(bool cond)
{
  if (cond)
    snprintf(listPtr, sizeof(listData) - (listPtr - listData), "               %s ","FALSE");
  else
    snprintf(listPtr, sizeof(listData) - (listPtr - listData), "               %s ","TRUE");

  return NORMAL;
}

// List error message
// Errors are always written to file if possible
// They are not turned off by NOLIST directive
int AsmCore::listError(const char *lineNum, const char *errMsg)
{
  if (!createdL68)
    return NORMAL;
  fprintf(listFile, "%s", lineNum);   // write line number to file (the original passes it as the format)
  fprintf(listFile, "%s", errMsg);    // write error message to file
  return NORMAL;
}

// List text
int AsmCore::listText(const char *text)
{
  if (!createdL68)
    return NORMAL;
  fprintf(listFile, "%s", text);        // write text to file
  return NORMAL;
}

int AsmCore::listObj(int data, int size)
{
  if (!CEXflag && (listPtr - listData + size > 31)) {
    strcpy(listData + ((size == WORD_SIZE) ? 26 : 28), "...");
    return NORMAL;
  }
  if (CEXflag && (listPtr - listData + size > 31)) {
    listLine(line);
    strcpy(listData, "          ");
    listPtr = listData + 10;
    continuation = true;
  }
  switch (size) {
    case BYTE_SIZE: snprintf(listPtr, sizeof(listData) - (listPtr - listData), "%02X ", data & 0xFF);
      listPtr += 3;
      break;
    case WORD_SIZE: snprintf(listPtr, sizeof(listData) - (listPtr - listData), "%04X ", data & 0xFFFF);
      listPtr += 5;
      break;
    case LONG_SIZE: snprintf(listPtr, sizeof(listData) - (listPtr - listData), "%08lX ", data);
      listPtr += 9;
      break;
    default: snprintf(buffer, sizeof(buffer), "LISTOBJ: INVALID SIZE CODE!\n");
      return MILD_ERROR;
  }

  return NORMAL;
}


int AsmCore::finishList()
{
  try {
    if (!createdL68)
      return NORMAL;
    putc('\n', listFile);
    if (errorCount > 0)
      fprintf(listFile, "%d error%s detected\n", errorCount,
                          (errorCount > 1) ? "s" : "");
    else {
      //***** DO NOT CHANGE THIS TEXT *****
      // "No error" is used by simulator to find the end of the code in the listing
      fprintf(listFile, "No errors detected\n");
    }
    if (warningCount > 0)
      fprintf(listFile, "%d warning%s generated\n", warningCount,
                          (warningCount > 1) ? "s" : "");
    else
      fprintf(listFile, "No warnings generated\n");

    // If OPT CRE Display Symbol Table ?
    if (CREflag)
      optCRE();   // Write symbol table to listing file

    // write starting address to first line of file
    // rewind to start of file and overwrite "00000000" with the address
    char start[16];
    std::snprintf(start, sizeof(start), "%08X", startAddress);
    listFile->data_.replace(0, 8, start);

    return NORMAL;
  }
  catch( ... ) {
    snprintf(buffer, sizeof(buffer), "ERROR: An exception occurred in routine 'finishList'. \n");
    printError(NULL, EXCEPTION, 0);
    return MILD_ERROR;
  }
}






}  // namespace easym68k::asm_::core
