* L1-06 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #'A',D0
        BSET    #5,D0
        SIMHALT
        END     START
