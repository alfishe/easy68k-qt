*-----------------------------------------------------------
* Solution O-12-EX1: Scale {10,20,30,40} by 3 and sum (300) in <= 220 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     ARR16(PC),A0
            MOVEQ   #0,D0
            MOVEQ   #4-1,D1
.LP:        MOVE.W  (A0)+,D2
            MOVE.W  D2,D3
            ADD.W   D2,D2
            ADD.W   D3,D2
            ADD.W   D2,D0
            DBRA    D1,.LP
            SIMHALT
ARR16:      DC.W    10,20,30,40
            END     START
