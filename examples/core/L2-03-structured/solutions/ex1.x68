* L2-03 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #0,D0
        REPEAT
            ADDQ.L  #3,D0
        UNTIL.L D0 <GE> #10 DO.S
        SIMHALT
        END     START
