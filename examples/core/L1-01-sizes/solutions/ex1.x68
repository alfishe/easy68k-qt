* L1-01 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVE.L  #$AABBCCF6,D0
        EXT.W   D0
        EXT.L   D0
        SIMHALT
        END     START
