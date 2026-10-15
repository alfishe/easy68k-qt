* L1-07 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #$3C,D0
        ROL.B   #4,D0
        SIMHALT
        END     START
