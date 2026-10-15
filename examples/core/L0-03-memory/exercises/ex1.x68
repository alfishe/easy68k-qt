* L0-03 Exercise 1: Store $11223344 at $2000 and read word at $2002 into D0
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #0,D0
        * TODO: write $11223344 to $2000 and read the word at $2002 into D0
        SIMHALT
        END     START
