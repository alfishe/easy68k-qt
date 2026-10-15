*-----------------------------------------------------------
* Title      : O-04 Naive Top-Tested Loop
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     ARR,A0
            MOVEQ   #0,D0
            MOVEQ   #0,D1
.LP:        CMPI.W  #16,D1
            BGE.S   .DONE
            ADD.W   (A0)+,D0
            ADDQ.W  #1,D1
            BRA.S   .LP
.DONE:      SIMHALT
ARR:        DC.W    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
            END     START
