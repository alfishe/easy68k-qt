*-----------------------------------------------------------
* Title      : O-03 Naive Indexed Copy
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     SRC,A0
            LEA     $2000,A1
            MOVEQ   #0,D1
            MOVEQ   #8-1,D2
.LP:        MOVE.W  0(A0,D1.W),0(A1,D1.W)
            ADDQ.W  #2,D1
            DBRA    D2,.LP
            SIMHALT
SRC:        DC.W    1,2,3,4,5,6,7,8
            END     START
