*-----------------------------------------------------------
* Solution C-09-EX1: TRAP #0 C handler + critical section -> 150
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            LEA     trap0_trampoline(PC),A0
            MOVE.L  A0,$0080
            TRAP    #0
            ORI.W   #$0700,SR
            ADD.L   #50,g_shared
            MOVE.L  g_shared,D0
            ANDI.W  #$F8FF,SR
            SIMHALT

trap0_trampoline:
            MOVEM.L D0-D1/A0-A1,-(A7)
            BSR.S   c_trap0_handler
            MOVEM.L (A7)+,D0-D1/A0-A1
            RTE

c_trap0_handler:
            MOVE.L  #100,g_shared
            RTS

            ORG     $2000
g_shared:   DS.L    1
            END     _start
