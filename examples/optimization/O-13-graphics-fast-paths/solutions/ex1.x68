*-----------------------------------------------------------
* Solution O-13-EX1: Fill (10,10)..(25,13) + Repaint in <= 120 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #87,D0
            MOVEQ   #10,D1
            MOVEQ   #10,D2
            MOVEQ   #25,D3
            MOVEQ   #13,D4
            TRAP    #15
            MOVEQ   #94,D0
            TRAP    #15
            SIMHALT
            END     START
