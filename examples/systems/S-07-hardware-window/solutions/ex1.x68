*-----------------------------------------------------------
* Solution S-07-EX1: Display '01' on the 7-segment display
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #1,D1
            TRAP    #15
            MOVEA.L D1,A0
            MOVE.B  #$3F,0(A0)
            MOVE.B  #$06,2(A0)
            SIMHALT
            END     START
