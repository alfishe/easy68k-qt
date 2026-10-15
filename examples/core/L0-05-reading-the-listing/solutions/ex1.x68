* L0-05 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
MAGIC   EQU     $1234
        ORG     $1000
START:
        MOVE.W  #MAGIC,D0
        MOVE.W  D0,$2000
        SIMHALT
        END     START
