* L3-02 Exercise 1: Find maximum of 5 words in DATA -> D0 = 91
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     DATA,A0
        MOVEQ   #0,D0
        * TODO: scan 5 words at (A0)+ and keep the largest in D0.W
        SIMHALT

DATA    DC.W    14,52,8,91,37
        END     START
