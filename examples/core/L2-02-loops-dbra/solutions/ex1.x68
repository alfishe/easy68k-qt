* L2-02 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #1,D0
        MOVEQ   #8-1,D1
LOOP:
        ADD.L   D0,D0
        DBRA    D1,LOOP
        SIMHALT
        END     START
