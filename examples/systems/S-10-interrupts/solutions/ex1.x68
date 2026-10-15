*-----------------------------------------------------------
* Solution S-10-EX1: Level 4 autovector ISR at $0070
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            LEA     IRQ4_ISR(PC),A0
            MOVE.L  A0,$0070
            MOVEQ   #32,D0
            MOVEQ   #6,D1
            MOVE.B  #$84,D2
            MOVEQ   #50,D3
            TRAP    #15
            STOP    #$2000
            MOVEQ   #32,D0
            MOVEQ   #6,D1
            MOVEQ   #0,D2
            TRAP    #15
            MOVE.L  D5,D0
            SIMHALT

IRQ4_ISR:   MOVEQ   #42,D5
            RTE
            END     START
