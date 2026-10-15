* L0-03 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #0,D0
        MOVE.L  #$11223344,$2000
        MOVE.W  $2002,D0
        SIMHALT
        END     START
