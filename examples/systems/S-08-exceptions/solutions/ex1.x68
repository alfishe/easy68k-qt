*-----------------------------------------------------------
* Solution S-08-EX1: TRAP #1 handler that doubles D0
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            LEA     TRAP1_ISR(PC),A0
            MOVE.L  A0,$0084
            MOVEQ   #15,D0
            TRAP    #1
            TRAP    #1
            SIMHALT

TRAP1_ISR:  ADD.L   D0,D0
            RTE
            END     START
