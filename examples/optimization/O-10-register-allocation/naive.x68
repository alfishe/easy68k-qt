*-----------------------------------------------------------
* Title      : O-10 Naive Stack-Spilling Subroutine
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            BSR.S   FIB10_NAIVE
            SIMHALT

FIB10_NAIVE:
            LINK    A6,#-12
            MOVEM.L D2-D7/A2-A5,-(A7)
            MOVE.L  #0,-4(A6)           ; a = 0
            MOVE.L  #1,-8(A6)           ; b = 1
            MOVE.W  #9,D2
.LP:        MOVE.L  -4(A6),D0
            ADD.L   -8(A6),D0
            MOVE.L  -8(A6),-4(A6)
            MOVE.L  D0,-8(A6)
            DBRA    D2,.LP
            MOVE.L  -8(A6),D0
            MOVEM.L (A7)+,D2-D7/A2-A5
            UNLK    A6
            RTS
            END     START
