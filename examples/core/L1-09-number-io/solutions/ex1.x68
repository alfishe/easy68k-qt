* L1-09 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #42,D1
        MOVEQ   #16,D2
        MOVEQ   #15,D0
        TRAP    #15
        SIMHALT
        END     START
