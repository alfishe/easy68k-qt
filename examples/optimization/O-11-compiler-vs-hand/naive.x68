*-----------------------------------------------------------
* Title      : O-11 Compiler -O1 Style Array Sum + Max
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     DATA,A0
            MOVEQ   #0,D0
            MOVE.W  (A0),D1
            MOVEQ   #0,D2
.LP:        CMPI.W  #8,D2
            BGE.S   .DONE
            MOVE.W  D2,D3
            ADD.W   D3,D3
            MOVE.W  0(A0,D3.W),D4
            EXT.L   D4
            ADD.L   D4,D0
            CMP.W   D1,D4
            BLE.S   .NEXT
            MOVE.W  D4,D1
.NEXT:      ADDQ.W  #1,D2
            BRA.S   .LP
.DONE:      SIMHALT
DATA:       DC.W    12,45,7,89,23,56,3,65
            END     START
