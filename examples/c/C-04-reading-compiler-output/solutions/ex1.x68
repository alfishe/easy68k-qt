*-----------------------------------------------------------
* Solution C-04-EX1: Dot product of A and B -> 70 in D0
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     VEC_A(PC),A0
            LEA     VEC_B(PC),A1
            MOVEQ   #0,D0
            MOVEQ   #4-1,D1
.lp:        MOVE.W  (A0)+,D2
            MULS.W  (A1)+,D2
            ADD.L   D2,D0
            DBRA    D1,.lp
            SIMHALT
VEC_A:      DC.W    1,2,3,4
VEC_B:      DC.W    5,6,7,8
            END     _start
