*-----------------------------------------------------------
* Solution S-03-EX1: MoveTo (40,60), LineTo (80,90), GetXY
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #86,D0
            MOVEQ   #40,D1
            MOVEQ   #60,D2
            TRAP    #15

            MOVEQ   #85,D0
            MOVEQ   #80,D1
            MOVEQ   #90,D2
            TRAP    #15

            MOVEQ   #96,D0
            TRAP    #15
            SIMHALT
            END     START
