*-----------------------------------------------------------
* Solution O-01-EX1: Sum 8 longwords at ARR into D0 in <= 220 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     ARR(PC),A0
            MOVEQ   #0,D0
            MOVEQ   #8-1,D1
.LP:        ADD.L   (A0)+,D0
            DBRA    D1,.LP
            SIMHALT

ARR:        DC.L    10,20,30,40,50,60,70,80
            END     START
