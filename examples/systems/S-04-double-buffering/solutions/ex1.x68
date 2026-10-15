*-----------------------------------------------------------
* Solution S-04-EX1: 5-frame pixel animation with Repaint
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #92,D0
            MOVEQ   #17,D1
            TRAP    #15

            MOVEQ   #10,D3
            MOVEQ   #5-1,D4
.LP:        MOVEQ   #82,D0
            MOVE.W  D3,D1
            MOVEQ   #25,D2
            TRAP    #15
            MOVEQ   #94,D0
            TRAP    #15
            MOVE.L  D3,D0
            ADD.L   #10,D3
            DBRA    D4,.LP
            SIMHALT
            END     START
