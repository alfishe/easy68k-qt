*-----------------------------------------------------------
* Solution O-07-EX1: Compute 13^2 = 169 in D0 in <= 48 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     SQ_TAB(PC),A0
            MOVEQ   #13*2,D1
            MOVEQ   #0,D0
            MOVE.W  0(A0,D1.W),D0
            SIMHALT

SQ_TAB:     DC.W    0,1,4,9,16,25,36,49,64,81,100,121,144,169,196,225
            END     START
