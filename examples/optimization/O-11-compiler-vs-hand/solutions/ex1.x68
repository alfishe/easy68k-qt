*-----------------------------------------------------------
* Solution O-11-EX1: Compute sum (D0=300) and max (D1=89) in <= 220 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     DATA(PC),A0
            MOVEQ   #0,D0
            MOVEQ   #0,D1
            MOVE.W  (A0)+,D0
            MOVE.W  D0,D1
            MOVEQ   #7-1,D2
.LP:        MOVE.W  (A0)+,D3
            ADD.W   D3,D0
            CMP.W   D1,D3
            BLE.S   .NEXT
            MOVE.W  D3,D1
.NEXT:      DBRA    D2,.LP
            SIMHALT
DATA:       DC.W    12,45,7,89,23,56,3,65
            END     START
