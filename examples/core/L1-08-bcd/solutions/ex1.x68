* L1-08 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #$37,D0
        MOVE    #0,CCR
        NBCD    D0
        SIMHALT
        END     START
