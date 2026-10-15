* L3-01 Exercise 1: Load 3rd longword (300) using 0(A0,D1.W)
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     LONGS,A0
        MOVEQ   #8,D1
        * TODO: load 0(A0,D1.W) into D0.L
        SIMHALT

LONGS   DC.L    100,200,300,400
        END     START
