*-----------------------------------------------------------
* Title      : O-08 Naive Q16.16 Shift Extraction
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVE.L  #$002A8000,D0       ; 42.5 in Q16.16
            MOVEQ   #16,D1
            ASR.L   D1,D0               ; 6 + 32 = 38 cycles just for shift!
            SIMHALT
            END     START
