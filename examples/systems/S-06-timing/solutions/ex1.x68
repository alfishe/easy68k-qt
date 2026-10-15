*-----------------------------------------------------------
* Solution S-06-EX1: Measure two delays (10 + 15 hundredths)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #8,D0
            TRAP    #15
            MOVE.L  D1,D2

            MOVEQ   #23,D0
            MOVEQ   #10,D1
            TRAP    #15

            MOVEQ   #23,D0
            MOVEQ   #15,D1
            TRAP    #15

            MOVEQ   #8,D0
            TRAP    #15
            MOVE.L  D1,D0
            SUB.L   D2,D0
            SIMHALT
            END     START
