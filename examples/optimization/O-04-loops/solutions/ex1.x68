*-----------------------------------------------------------
* Solution O-04-EX1: Sum 16 words at ARR into D0 in <= 200 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     ARR(PC),A0
            MOVEQ   #0,D0
            MOVEQ   #4-1,D1
.LP:        ADD.W   (A0)+,D0
            ADD.W   (A0)+,D0
            ADD.W   (A0)+,D0
            ADD.W   (A0)+,D0
            DBRA    D1,.LP
            SIMHALT
ARR:        DC.W    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
            END     START
