*-----------------------------------------------------------
* Title      : O-07 Naive Bit-by-Bit Popcount Loop
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVE.B  #$D5,D1
            MOVEQ   #0,D0
            MOVEQ   #8-1,D2
.LP:        LSR.B   #1,D1
            BCC.S   .SKIP
            ADDQ.B  #1,D0
.SKIP:      DBRA    D2,.LP
            SIMHALT
            END     START
