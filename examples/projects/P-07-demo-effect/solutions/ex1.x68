*-----------------------------------------------------------
* Solution P-07-EX1: Copy 16-byte SIN16 scanline to $2000 and sum to D0=1024 in <= 400 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     SIN16(PC),A0
            LEA     $2000,A1
            MOVEQ   #0,D0
            MOVEQ   #0,D2
            MOVEQ   #16-1,D1
.LP:        MOVE.B  (A0)+,D2
            MOVE.B  D2,(A1)+
            ADD.W   D2,D0
            DBRA    D1,.LP
            SIMHALT
SIN16:      DC.B    64,88,110,123,128,123,110,88
            DC.B    64,40,18,5,0,5,18,40
            END     START
