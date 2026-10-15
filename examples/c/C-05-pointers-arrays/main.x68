*-----------------------------------------------------------
* Title      : C-05 Pointers, Arrays, and volatile MMIO
* Description: Annotated 68000 assembly for main.c
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $00FF0000,A7
            BSR.S   main
            SIMHALT

main:
            LEA     kValues(PC),A0
            MOVEQ   #0,D0
            MOVEQ   #4-1,D1
.lp:        ADD.L   (A0)+,D0
            DBRA    D1,.lp
            MOVE.B  D0,$00E00010        ; volatile MMIO write to LEDs ($96)
            RTS

kValues:    DC.L    15,30,45,60
            END     _start
