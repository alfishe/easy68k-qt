// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from Edit68K/DIRECTIV.CPP of the original EASy68K (Paul McKee, Charles
// Kelly, Tim Larson). The functions follow the original line by line; the
// original's global state lives in AsmCore (see asm_core.h).

#include "asm_core.h"

namespace easym68k::asm_::core {

/***********************************************************************
 *
 *		DIRECTIVE.C
 *		Directive Routines for 68000 Assembler
 *
 * Description: The functions in this file carry out the functions of
 *		assembler directives. All the functions share the same
 *		calling sequence:
 *
 *		    general_name(size, label, op, errorPtr)
 *		    int size;
 *		    char *label, *op;
 *		    int *errorPtr;
 *
 *		The size argument contains the size code that was
 *		specified with the instruction (using the definitions
 *		in ASM.H) or 0 if no size code was specified. The label
 *		argument is a pointer to a string (which may be empty)
 *		containing the label from the line containing the
 *		directive. The op argument is a pointer to the first
 *		non-blank character after the name of the directive,
 *		i.e., the operand(s) of the directive. The errorPtr
 *		argument is used to return a status via the standard
 *		mechanism.
 *
 *      Author: Paul McKee
 *		ECE492    North Carolina State University
 *
 *        Date:	12/13/86
 *
 *    Modified: Charles Kelly
 *              Monroe County Community College
 *              http://www.monroeccc.edu/ckelly
 ************************************************************************/





// listPtr: pointer to the buffer where the listing line is assembled
// (used to put =XXXXXXXX in the listing for EQU's and SET's).




/***********************************************************************
 *	ORG directive.
 ***********************************************************************/

int AsmCore::org(int size, char *label, char *op, int *errorPtr)
{
  int newLoc;
  bool backRef;

  if (size)
    NEWERROR(*errorPtr, INV_SIZE_CODE);
  if (!*op) {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }
  if (offsetMode) {
    loc = locOffset;            // restore loc used prior to Offset directive
    offsetMode = false;         // turn off offset mode
  }

  op = eval(op, &newLoc, &backRef, errorPtr);
  if (*errorPtr < SEVERE && !backRef) {
    NEWERROR(*errorPtr, INV_FORWARD_REF);
  }
  else if (*errorPtr < ERRORN) {
    if (isspace(*op) || !*op) {
      // Check for an odd value, adjust to one higher
      if (newLoc & 1) {
	NEWERROR(*errorPtr, ODD_ADDRESS);
	newLoc++;
      }
      loc = newLoc;
      // Define the label attached to this directive, if any
      if (*label)
	define(label, loc, pass2, true, errorPtr);
      // Show new location counter on listing
      listLoc();
    }
    else
      NEWERROR(*errorPtr, SYNTAX);
  }
  return NORMAL;
}

/***********************************************************************
 *	SECTION directive (ck)
 ***********************************************************************/
int AsmCore::section(int size, char *label, char *op, int *errorPtr)
{
  int value;
  int newLoc;
  bool backRef = true;

  if (size)
    NEWERROR(*errorPtr, INV_SIZE_CODE);

  if (offsetMode) {
    loc = locOffset;            // restore loc used prior to Offset directive
    offsetMode = false;         // turn off offset mode
  }
  if (*op) {                    // if section number
    eval(op, &value, &backRef, errorPtr);    // evaluate section number
    if (*errorPtr < SEVERE && !backRef) {
      NEWERROR(*errorPtr, INV_FORWARD_REF);
    }
    else if (*errorPtr < ERRORN) {
      if (value < 0 || value > 15) {              // if illegal section number
        NEWERROR(*errorPtr, SYNTAX);
        return NORMAL;
      }
      sectionLoc[sectI] = loc;            // save current loc to current section
      loc = sectionLoc[value];            // set loc to new section
      sectI = value;                      // new section number
      // Show new location counter on listing
      listLoc();
    } else {
      NEWERROR(*errorPtr, SYNTAX);
    }
  } else if (!*label)           // else no section number if no label
    NEWERROR(*errorPtr, LABEL_REQUIRED);
  if (*label)                 // if label
    define(label, sectI, pass2, true, errorPtr);  // define to sectI
  return NORMAL;
}

/***********************************************************************
 *	OFFSET directive. (ck)
 ***********************************************************************/
int AsmCore::offset(int size, char *label, char *op, int *errorPtr)
{
  int newLoc;
  bool backRef;

  if (size)
    NEWERROR(*errorPtr, INV_SIZE_CODE);
  if (!*op) {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }
  if (*label)                           // if label
    NEWERROR(*errorPtr, LABEL_ERROR);

  op = eval(op, &newLoc, &backRef, errorPtr);
  if (*errorPtr < SEVERE && !backRef) {
    NEWERROR(*errorPtr, INV_FORWARD_REF);
  }
  else if (*errorPtr < ERRORN) {
    if (isspace(*op) || !*op) {
      if (!offsetMode) {     // if not currently processing an Offset directive
        locOffset = loc;        // save current location
        offsetMode = true;
      }
      loc = newLoc;
      // Show new location counter on listing
      listLoc();
    }
    else
      NEWERROR(*errorPtr, SYNTAX);
  }
  return NORMAL;
}

/***********************************************************************
 *	END directive.
 ***********************************************************************/
 // modified by CK
 // Allows starting address to be specified as operand for compatibility
 // with Teeside emulator. Defaults to starting address of $1000.

int AsmCore::funct_end(int size, char *label, char *op, int *errorPtr)
{
  int newLoc;
  bool backRef;

  if (size)
    NEWERROR(*errorPtr, INV_SIZE_CODE);
  endFlag = true;

  if (!*op) {                   // if no address specified
    startAddress = 0x1000;      // assume starting address of $1000
    NEWERROR(*errorPtr, ADDRESS_MISSING);
    return NORMAL;
  }
  if (*label)                           // if label
    NEWERROR(*errorPtr, LABEL_ERROR);

  op = eval(op, &newLoc, &backRef, errorPtr);
  if (*errorPtr < SEVERE && !backRef) {
    NEWERROR(*errorPtr, INV_FORWARD_REF);
  }
  else if (*errorPtr < ERRORN) {
    if (isspace(*op) || !*op) {
      /* Check for an odd value, adjust to one higher */
      if (newLoc & 1) {
	NEWERROR(*errorPtr, ODD_ADDRESS);
	newLoc++;
      }
      startAddress = newLoc;
    }
    else
      NEWERROR(*errorPtr, SYNTAX);
  }

  return NORMAL;
}


/***********************************************************************
 *	EQU directive.
 ***********************************************************************/

int AsmCore::equ(int size, char *label, char *op, int *errorPtr)
{
  int value;
  bool backRef;
  char *op1;

  if (size)
    NEWERROR(*errorPtr, INV_SIZE_CODE);
  if (!*op) {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }

  op = eval(op, &value, &backRef, errorPtr);
  if (*errorPtr < SEVERE && !backRef) {
    NEWERROR(*errorPtr, INV_FORWARD_REF);
  }
  else if (*errorPtr < ERRORN)
    if (isspace(*op) || !*op)
      if (!*label) {
	NEWERROR(*errorPtr, LABEL_REQUIRED);
      }	else {
	define(label, value, pass2, true, errorPtr);
	if (pass2 && listFlag && *errorPtr < MINOR) {
	  snprintf(listPtr, sizeof(listData) - (listPtr - listData), "=%08lX ", value);
	  listPtr += 10;
	}
      }
    else
      NEWERROR(*errorPtr, SYNTAX);

  return NORMAL;
}


/***********************************************************************
 *	SET directive.
 ***********************************************************************/

int AsmCore::set(int size, char *label, char *op, int *errorPtr)
{
  int value;
  int  error;
  bool backRef;
  symbolDef *symbol;

  if (size)
    NEWERROR(*errorPtr, INV_SIZE_CODE);
  if (!*op) {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }
  error = OK;
  op = eval(op, &value, &backRef, errorPtr);
//  if (*errorPtr < SEVERE && !backRef) {       // ck 4-18-03
//    NEWERROR(*errorPtr, INV_FORWARD_REF);
//  }
//  else
   if (*errorPtr < ERRORN)          // ck 4-18-03
    if (isspace(*op) || !*op)
      if (!*label) {
	NEWERROR(*errorPtr, LABEL_REQUIRED);
      } else {
	error = OK;
	symbol = define(label, value, pass2, false, &error);   // ck 4-18-03
	symbol->flags |= REDEFINABLE;
	if (pass2 & listFlag) {
	  snprintf(listPtr, sizeof(listData) - (listPtr - listData), "=%08lX ", value);
	  listPtr += 10;
	}
      }
    else
      NEWERROR(*errorPtr, SYNTAX);

  return NORMAL;
}


/***********************************************************************
 *	DC directive.
 ***********************************************************************/

int AsmCore::dc(int size, char *label, char *op, int *errorPtr)
{
  int	outVal;
  bool backRef;
  char string[260], *p;

  if (size == SHORT_SIZE) {
    NEWERROR(*errorPtr, INV_SIZE_CODE);
    size = WORD_SIZE;
  }
  else if (!size)
    size = WORD_SIZE;

  // Move location counter to a word boundary and fix the listing if doing
  // DC.W or DC.L (but not if doing DC.B, so DC.B's can be contiguous)
  if ((size & (WORD_SIZE | LONG_SIZE)) && (loc & 1)) {
    loc++;
  }
  showEqual = true;
  listLoc();
  showEqual = false;
  // Define the label attached to this directive, if any
  if (*label)
    define(label, loc, pass2, true, errorPtr);
  // Check for the presence of the operand list
  if (!*op) {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }
  do {
    p = op;                             // save location of op
    if (*op == '\'') {                  // if string
      op = collect(++op, string);       // copy to string
      if (!isspace(*op) && *op != ',') { // if stuff after string
        op = p;                         // restore op
        goto notString;                 // ***** GOTO *****
      }
      p = string;
      while (*p) {
	outVal = *p++;
	if (size > BYTE_SIZE)
	  outVal = (outVal << 8) + *p++;
	if (size > WORD_SIZE) {
	  outVal = (outVal << 16) + (*p++ << 8);
	  outVal += *p++;
	}
	if (pass2)
	  output(outVal, size);
	loc += size;
      }
    } else {

notString:                              // process non strings
      op = skipSpace(op);               // skip spaces
      op = eval(op, &outVal, &backRef, errorPtr);
      if (*errorPtr > SEVERE)
	return NORMAL;
      //if (!isspace(*op) && *op != ',') {
      //	NEWERROR(*errorPtr, SYNTAX);
      //	return NORMAL;
      //}
      if (pass2)
	output(outVal, size);
      loc += size;
      if (size == BYTE_SIZE && (outVal < -128 || outVal > 255)) {
	NEWERROR(*errorPtr, INV_8_BIT_DATA);
      }
      else if (size == WORD_SIZE && (outVal < -32768 || outVal > 65535))
	NEWERROR(*errorPtr, INV_16_BIT_DATA);
    }
    op = skipSpace(op);               // skip spaces
  } while (*op++ == ',');
//  --op;
//  if (!isspace(*op) && *op)
//    NEWERROR(*errorPtr, SYNTAX);

  return NORMAL;
}

/**********************************************************************
 *	Function collect parses strings for dc. Each output string
 *	is padded with four nulls at the end.
 **********************************************************************/

char *AsmCore::collect(char *s, char *d)
{
  while (*s) {
    if (*s == '\'')
      if (*(s+1) == '\'') {
	*d++ = *s;
	s += 2;
      } else {
	*d++ = '\0';
	*d++ = '\0';
	*d++ = '\0';
	*d = '\0';
	return ++s;
      }
    else
      *d++ = *s++;
  }
  return s;
}


/***********************************************************************
 *	DCB directive
 ***********************************************************************/

int AsmCore::dcb(int size, char *label, char *op, int *errorPtr)
{
  int	blockSize, blockVal, i;
  bool backRef;

  if (size == SHORT_SIZE) {
    NEWERROR(*errorPtr, INV_SIZE_CODE);
    size = WORD_SIZE;
  }
  else if (!size)
    size = WORD_SIZE;

  // Move location counter to a word boundary and fix the listing if doing
  // DCB.W or DCB.L (but not if doing DCB.B, so DCB.B's can be contiguous)

  if ((size & (WORD_SIZE | LONG_SIZE)) && (loc & 1)) {
    loc++;
  }
  showEqual = true;
  listLoc();
  showEqual = false;

  // Define the label attached to this directive, if any
  if (*label)
    define(label, loc, pass2, true, errorPtr);
  // Evaluate the size of the block (in bytes, words, or longwords)
  op = eval(op, &blockSize, &backRef, errorPtr);

  if (*errorPtr < SEVERE && !backRef) {
    NEWERROR(*errorPtr, INV_FORWARD_REF);
    return NORMAL;
  }
  if (*errorPtr > SEVERE)
    return NORMAL;
  if (*op != ',') {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }
  if (blockSize < 0 || ((loc + (blockSize * size)) > MEM_SIZE)) {
    NEWERROR(*errorPtr, INV_LENGTH);
    return NORMAL;
  }
  // Evaluate the data to put in block
  op = eval(++op, &blockVal, &backRef, errorPtr);
  if (*errorPtr < SEVERE) {
    if (!isspace(*op) && *op) {
      NEWERROR(*errorPtr, SYNTAX);
      return NORMAL;
    }
    // On pass 2, output the block of values
    if (pass2) {
      bool CEXsave = CEXflag;
      CEXflag = false;          // prevent display of all DCB data
      for (i = 0; i < blockSize; i++) {
        output(blockVal, size);
	loc += size;
      }
      CEXflag = CEXsave;
    }else
      loc += blockSize * size;
  }

  return NORMAL;
}


/***********************************************************************
 *	DS directive.
 ***********************************************************************/

int AsmCore::ds(int size, char *label, char *op, int *errorPtr)
{
  int	blockSize;
  bool	backRef;

  if (size == SHORT_SIZE) {
    NEWERROR(*errorPtr, INV_SIZE_CODE);
    size = WORD_SIZE;
  }
  else if (!size)
    size = WORD_SIZE;

  // Move location counter to a word boundary and fix the listing if doing
  // DS.W or DS.L (but not if doing DS.B, so DS.B's can be contiguous)
  if ((size & (WORD_SIZE | LONG_SIZE)) && (loc & 1)) {
    loc++;
    listLoc();
  }

  // Define the label attached to this directive, if any
  if (*label)
    define(label, loc, pass2, true, errorPtr);
  // Evaluate the size of the block (in bytes, words, or longwords)
  op = eval(op, &blockSize, &backRef, errorPtr);
  if (*errorPtr < SEVERE && !backRef) {
    NEWERROR(*errorPtr, INV_FORWARD_REF);
    return NORMAL;
  }
  if (*errorPtr > SEVERE)
    return NORMAL;
  if (!isspace(*op) && *op) {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }
  if (blockSize < 0) {
    NEWERROR(*errorPtr, INV_LENGTH);
    return NORMAL;
  }
  loc += blockSize * size;

  return NORMAL;
}

//**********************************************************************
//	OPT directive
//**********************************************************************

int AsmCore::opt(int size, char *label, char *op, int *errorPtr)
{
  int i;
  const int OPTCHARS = 8;       // max number of characters in OPT operand
  char option[OPTCHARS+1];
  bool done = false;

  if (size)
    NEWERROR(*errorPtr, INV_SIZE_CODE);
  if (!*op) {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }
  if (*label)                           // if label
    NEWERROR(*errorPtr, LABEL_ERROR);

  op = skipSpace(op);                   // skip spaces
  if (!*op) {           // if no option specified
    NEWERROR(*errorPtr, INV_OPCODE);
    return NORMAL;
  }

  while (isalnum(*op) && !done)
  {
    i = 0;
    do {
      if (i < OPTCHARS)           // only first OPTCHARS of operand are used
        option[i++] = *op;
      op++;
    } while (isalnum(*op));
    option[i] = '\0';             // end option string with null
    op = skipSpace(op);           // skip spaces
    if(*op == ',') {
      op++;                       // skip comma between options
      op = skipSpace(op);         // skip spaces
    } else
      done = true;                // end of options

    if ( strcmpi(option,"CRE") == 0)              // if CRE option
      CREflag = true;             // enable CRE (display symbol table in listing)
    else if ( strcmpi(option,"MEX") == 0)
      MEXflag = true;             // enable macro expansion
    else if ( strcmpi(option,"NOMEX") == 0)
      MEXflag = false;            // disable macro expansion
    else if ( strcmpi(option,"SEX") == 0)
      SEXflag = true;             // enable structured expansion
    else if ( strcmpi(option,"NOSEX") == 0)
      SEXflag = false;            // disable structured expansion
    else if ( strcmpi(option,"WAR") == 0)
      WARflag = true;             // show warnings
    else if ( strcmpi(option,"NOWAR") == 0)
      WARflag = false;            // suppress warnings
    else if ( strcmpi(option,"CEX") == 0)
      CEXflag = true;             // enable constant expansion
    else if ( strcmpi(option,"NOCEX") == 0)
      CEXflag = false;            // disable constant expansion
    else if ( strcmpi(option,"BIT") == 0) {
      BITflag = true;             // enable bit field support
      if (pass2 && listFlag) {
        listLine("*[sim68k]bitfield\n");        // enables bit field support in Sim68K
      }
    }
    else
      NEWERROR(*errorPtr, SYNTAX);
  }
  return NORMAL;
}

//--------------------------------------------------------
//         INCLUDE directive (ck)
// Inserts specified source file into current file and assembles
// for (each line of new file) {
//   assemble this line
// }
int AsmCore::include(int size, char *label, char *fileName, int *errorPtr)
{
  char incLine[256], capLine[256];
  char *src, *dst;
  int error, i;
  int tempFP;                          // temporary File Pointer
  bool comment;                         // true when line is comment
  int value;
  bool backRef;
  MemFile *incFile;
  MemFile *tmpInFile;                   // save current file
  char quote;
  int lineNumInc;                       // line number for include file
  int lineNumSave;
  char fileNameSave[256];

  if (size) {                                   // if .size code specified
    NEWERROR(*errorPtr, INV_SIZE_CODE);         // error, invalid size code
    return NORMAL;
  }

  if (!*fileName) {
    NEWERROR(*errorPtr, FILE_ERROR);         // error, invalid syntax
    return NORMAL;
  }

  // Define the label attached to this include directive, if any
  if (*label)
    define(label, loc, pass2, true, errorPtr);

  strcap(capLine, fileName);

  // strip quotes from filename
  src = capLine;
  dst = capLine;
  src = skipSpace(src);         // skip leading spaces in filename
  if (*src == '\"' || *src == '\'') {       // *ck 12-9-2005
    quote = *src;
    src++;
  } else
    quote = '\0';
  while (*src && (*src != ' ' || quote) ) {
    if (*src == quote)
      break;
    else
      *dst++ = *src++;
  }

  // strip whitespace from end of filename
  dst--;
  while (dst > capLine && isspace(*dst))
    dst--;
  dst++;
  *dst = '\0';

  if (pass2 && listFlag) {   // if include directive should be listed
    listLine(line);
  }

  try {
    incFile = OpenFile(capLine, fileName);  // attempt to open include file
    if (!incFile) {                    // if error opening file
      NEWERROR(*errorPtr, FILE_ERROR);     // error, invalid syntax
      return SEVERE;
    }
    tmpInFile = inFile;                 // save current input file
    inFile = incFile;                   // make include file input file
    strcpy(fileNameSave,includeFile);   // save current include file
    strcpy(includeFile,capLine);        // save new include file
    lineNumSave = lineNum;              // save current line number

    // (The original clears the message pane of an editor window that has
    // the included file open.)

    // assemble each line of the include file
    // until END directive or EOF
    includeNestLevel++;                 // count nest level of include directive
    lineNum = 1;
    while(!endFlag && fgets(line, 256, inFile)) {
      error = OK;
      skipList = false;
      continuation = false;
      printCond = false;           // true to print condition on listing line
      if(line[0] != '*' && line[1] != '~')      // don't assemble font info

        assemble(line, &error);
        lineNum++;
    }
    inFile = tmpInFile;                 // restore previous input file
    strcpy(includeFile,fileNameSave);   // restore previous include file
    lineNum = lineNumSave;              // restore line number

    includeNestLevel--;

    if (pass2 && listFlag) {
      skipList = true;      // don't list INCLUDE statement again
      listLine("-------------------- end include --------------------\n");
    }

  }
  catch( ... ) {
    snprintf(buffer, sizeof(buffer), "ERROR: An exception occurred in routine 'include'. \n");
    printError(NULL, EXCEPTION, 0);
    return NULL;
  }
  return NORMAL;
}

//--------------------------------------------------------
//         INCBIN directive (ck)
// Inserts specified binary file into S-Record and Binary output file.
// Does not process the data in any way.

int AsmCore::incbin(int size, char *label, char *fileName, int *errorPtr)
{
  char capLine[256];
  char *src, *dst;
  MemFile *incFile;
  char quote;
  unsigned char dataByte;

  if (size) {                                   // if .size code specified
    NEWERROR(*errorPtr, INV_SIZE_CODE);         // error, invalid size code
    return NORMAL;
  }

  if (!*fileName) {
    NEWERROR(*errorPtr, FILE_ERROR);         // error, invalid syntax
    return NORMAL;
  }

  // Define the label attached to this include directive, if any
  if (*label)
    define(label, loc, pass2, true, errorPtr);

  strcap(capLine, fileName);

  // strip quotes from filename
  src = capLine;
  dst = capLine;
  src = skipSpace(src);         // skip leading spaces in filename
  if (*src == '\"' || *src == '\'') {
    quote = *src;
    src++;
  } else
    quote = '\0';
  while (*src && (*src != ' ' || quote) ) {
    if (*src == quote)
      break;
    else
      *dst++ = *src++;
  }

  // strip whitespace from end of filename
  dst--;
  while (dst > capLine && isspace(*dst))
    dst--;
  dst++;
  *dst = '\0';

  if (pass2 && listFlag) {   // if incbin directive should be listed
    listLine(line);
  }

  try {
    incFile = OpenFile(capLine, fileName);  // attempt to open incbin binary file
    if (!incFile) {                    // if ERROR opening file
      NEWERROR(*errorPtr, FILE_ERROR);     // error, invalid syntax
      return SEVERE;
    }

    // loop through every byte of incbin file
    // read 1 byte of data from file
    while(fread(&dataByte, 1, 1, incFile) && !feof(incFile)) {
      // On pass 2, output the block of values directly
      // to the object and binary files (without putting them in the listing)
      if (pass2) {
        if (objFlag)
          outputObj(loc, dataByte, BYTE_SIZE);
      }
      loc++;            // increment location counter once for each byte in file
    }
    if (pass2 && listFlag) {
      skipList = true;      // don't list INCBIN statement again
    }

  }
  catch( ... ) {
    snprintf(buffer, sizeof(buffer), "ERROR: An exception occurred in routine 'incbin'. \n");
    printError(NULL, EXCEPTION, 0);
    return NULL;
  }
  return NORMAL;
}

//--------------------------------------------------------
//         FAIL directive (ck)
// Displays the message following the FAIL directive.
// Increments the Assemblers error count just like any other error.
//
int AsmCore::failError(int size, char *label, char *message, int *errorPtr)
{
  if (size) {                                   // if .size code specified
    NEWERROR(*errorPtr, INV_SIZE_CODE);         // error, invalid size code
    return NORMAL;
  }

  // Define the label attached to this include directive, if any
  if (*label)
    define(label, loc, pass2, true, errorPtr);

  if (!*message)                                // if no message
    snprintf(buffer, sizeof(buffer), "ERROR: Unspecified user defined error. \n");
  else {
    strncpy(buffer, message, 254);
    buffer[255] = '\0';
  }
  NEWERROR(*errorPtr, FAIL_ERROR);

  // Automatically listed because it's treated like an error

  return NORMAL;
}

//--------------------------------------------------------
//         LIST directive (ck)
// Turns on listing of assembled code to .L68 file
//
int AsmCore::listOn(int size, char *label, char *message, int *errorPtr)
{
  listFlag = true;
  skipList = true;      // don't display LIST directive
  return NORMAL;
}

//--------------------------------------------------------
//         NOLIST directive (ck)
// Turns off listing of assembled code to .L68 file
//
int AsmCore::listOff(int size, char *label, char *message, int *errorPtr)
{
  listFlag = false;
  return NORMAL;
}

//**********************************************************************
//	MEMORY directive
//**********************************************************************

int AsmCore::memory(int size, char *label, char *op, int *errorPtr)
{
  int i;
  const int MAXCHARS = 10;       // max number of characters in operand
  char operand[MAXCHARS+1];
  bool ROMmap = false, readMap = false, protectedMap = false, invalidMap = false;
  int startAddr = 0, endAddr = 0;
  bool	backRef;

  if (!pass2)                   // runs during pass2
    return NORMAL;

  if (size)
    NEWERROR(*errorPtr, INV_SIZE_CODE);
  if (!*op) {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }
  if (*label)                           // if label
    NEWERROR(*errorPtr, LABEL_ERROR);

  op = skipSpace(op);                   // skip spaces
  if (!*op) {           // if no option specified
    NEWERROR(*errorPtr, INV_OPCODE);
    return NORMAL;
  }

  i = 0;
  do {
    if (i < MAXCHARS)           // only first MAXCHARS of operand are used
      operand[i++] = *op;
    op++;
  } while (isalnum(*op));
  operand[i] = '\0';            // end operand string with null
  op = skipSpace(op);           // skip spaces

  if ( strcmpi(operand,"ROM") == 0)            // if ROM map
    ROMmap = true;
  else if ( strcmpi(operand,"READ") == 0)      // if Read map
    readMap = true;
  else if ( strcmpi(operand,"PROTECTED") == 0) // if Protected map
    protectedMap = true;
  else if ( strcmpi(operand,"INVALID") == 0)   // if Invalid map
    invalidMap = true;
  else {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }

  // Evaluate the start address
  op = eval(op, &startAddr, &backRef, errorPtr);
  if (*errorPtr < SEVERE && !backRef) {
    NEWERROR(*errorPtr, INV_FORWARD_REF);
    return NORMAL;
  }
  if (*errorPtr > SEVERE)
    return NORMAL;
  op = skipSpace(op);           // skip spaces
  if (*op != ',') {
    NEWERROR(*errorPtr, SYNTAX);
    return NORMAL;
  }
  op++;                         // skip ','
  op = skipSpace(op);           // skip spaces
  // Evaluate the end address
  eval(op, &endAddr, &backRef, errorPtr);
  if (*errorPtr < SEVERE && !backRef) {
    NEWERROR(*errorPtr, INV_FORWARD_REF);
    return NORMAL;
  }
  if (*errorPtr > SEVERE)
    return NORMAL;

  if (ROMmap) {
    mapROM = true;
    mapROMStart = startAddr & 0x00FFFFFF;
    mapROMEnd = endAddr & 0x00FFFFFF;
  } else if (readMap) {
    mapRead = true;
    mapReadStart = startAddr & 0x00FFFFFF;
    mapReadEnd = endAddr & 0x00FFFFFF;
  } else if (protectedMap) {
    mapProtected = true;
    mapProtectedStart = startAddr & 0x00FFFFFF;
    mapProtectedEnd = endAddr & 0x00FFFFFF;
  } else if (invalidMap) {
    mapInvalid = true;
    mapInvalidStart = startAddr & 0x00FFFFFF;
    mapInvalidEnd = endAddr & 0x00FFFFFF;
  }

  return NORMAL;
}

//**********************************************************************
//	SIMHALT directive
//**********************************************************************
int AsmCore::simhalt(int size, char *label, char *op, int *errorPtr)
{
  if (size)
    NEWERROR(*errorPtr, INV_SIZE_CODE);

  // Move location counter to a word boundary
  if (loc & 1)
    loc++;

  listLoc();

  // Define the label attached to this directive, if any
  if (*label)
    define(label, loc, pass2, true, errorPtr);

  if (pass2) {
    //if (listFlag)       // if directive should be listed
    //  listLine(line);
    // Port fix: the original outputs both words at the same address, so only
    // one $FFFF reached the S-record (the second came from Sim68K's erased
    // memory). Advance the location between the two words.
    output(0xFFFF, WORD_SIZE);  // opcode $FFFF $FFFF
    loc += WORD_SIZE;
    output(0xFFFF, WORD_SIZE);  // opcode $FFFF $FFFF
    loc -= WORD_SIZE;
  }
  loc += LONG_SIZE;

  return NORMAL;
}

//**********************************************************************
//	PAGE directive
// Adds NEW_PAGE_MARKER to the .L68 file
//**********************************************************************
int AsmCore::page(int size, char *label, char *op, int *errorPtr)
{
  if (size)
    NEWERROR(*errorPtr, INV_SIZE_CODE);

  if (*label)                           // if label
    NEWERROR(*errorPtr, LABEL_ERROR);

  skipList = true;                      // don't display PAGE directive

  if (pass2 && listFlag) {
    listText(NEW_PAGE_MARKER);
    listText("\n");
  }

  return NORMAL;
}


}  // namespace easym68k::asm_::core
