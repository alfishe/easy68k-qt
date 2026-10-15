*-----------------------------------------------------------
* Title      : C-02 C Types on the 68000
* Description: Annotated 68000 assembly for main.c
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $00FF0000,A7
            BSR.S   main
            SIMHALT

main:
            MOVE.L  #$01020404,D0
            MOVE.L  D0,$2000
            MOVE.L  #$12345678,$2004
            RTS

            END     _start
