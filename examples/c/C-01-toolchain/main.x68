*-----------------------------------------------------------
* Title      : C-01 Freestanding C Toolchain (crt0 + main)
* Description: Annotated 68000 assembly for main.c
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            * crt0: initialise stack and zero .bss [__bss_start .. __bss_end)
            LEA     $00FF0000,A7
            LEA     __bss_start,A0
            LEA     __bss_end,A1
.bss_lp:    CMPA.L  A1,A0
            BHS.S   .call_main
            CLR.B   (A0)+
            BRA.S   .bss_lp

.call_main:
            BSR.S   main
            SIMHALT

main:
            LINK    A6,#0
            LEA     MSG(PC),A1
            MOVEQ   #13,D0
            TRAP    #15
            MOVE.L  g_base,D0
            ADD.L   g_uninit,D0
            ADDQ.L  #2,D0
            UNLK    A6
            RTS

MSG:        DC.B    'C-01: 42',0
            DS.W    0

            ORG     $2000
g_base:     DC.L    40
__bss_start:
g_uninit:   DS.L    1
__bss_end:
            END     _start
