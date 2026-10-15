*-----------------------------------------------------------
* Solution S-11-EX1: Read user stack from supervisor TRAP #0
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            LEA     TRAP0(PC),A0
            MOVE.L  A0,$0080
            LEA     $00FE0000,A0
            MOVE.L  A0,USP
            ANDI.W  #$DFFF,SR
            MOVE.L  #$12345678,-(A7)
            TRAP    #0

TRAP0:      MOVE.L  USP,A0
            MOVE.L  (A0),D0
            SIMHALT
            END     START
