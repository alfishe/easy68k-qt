*-----------------------------------------------------------
* Title      : C-07 libeasy68k Inline Assembly Runtime
* Description: Annotated 68000 assembly for main.c
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $00FF0000,A7
            BSR.S   main
            SIMHALT

main:
            LEA     STR_VAL(PC),A1
            MOVEQ   #14,D0
            TRAP    #15

            MOVE.L  #255,D1
            MOVEQ   #3,D0
            TRAP    #15

            LEA     STR_HEX(PC),A1
            MOVEQ   #14,D0
            TRAP    #15

            MOVE.L  #255,D1
            MOVEQ   #16,D2
            MOVEQ   #15,D0
            TRAP    #15

            LEA     STR_EMPTY(PC),A1
            MOVEQ   #13,D0
            TRAP    #15

            MOVE.L  #255,D0
            RTS

STR_VAL:    DC.B    'val=',0
STR_HEX:    DC.B    ' hex=',0
STR_EMPTY:  DC.B    0
            DS.W    0
            END     _start
