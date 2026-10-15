* L3-04 Exercise 1: Sort two words at $2000 and $2002 in ascending order
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVE.W  #30,$2000
        MOVE.W  #10,$2002
        * TODO: compare and swap if $2000 > $2002
        SIMHALT
        END     START
