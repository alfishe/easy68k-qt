* L3-08 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
SWAP_REG MACRO
        EXG     \1,\2
        ENDM

        ORG     $1000
START:
        MOVEQ   #11,D0
        MOVEQ   #22,D1
        SWAP_REG D0,D1
        SIMHALT
        END     START
