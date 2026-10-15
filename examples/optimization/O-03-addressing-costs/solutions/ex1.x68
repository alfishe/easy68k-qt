*-----------------------------------------------------------
* Solution O-03-EX1: Copy 8 words from SRC to $2000 in <= 200 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     SRC(PC),A0
            LEA     $2000,A1
            MOVEQ   #8-1,D2
.LP:        MOVE.W  (A0)+,(A1)+
            DBRA    D2,.LP
            SIMHALT
SRC:        DC.W    11,22,33,44,55,66,77,88
            END     START
