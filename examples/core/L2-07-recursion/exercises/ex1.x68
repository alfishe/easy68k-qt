* L2-07 Exercise 1: Recursive triangular number TRI(6) = 21
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #6,D0
        BSR.S   TRI
        SIMHALT

TRI:
        * TODO: if D0 <= 1 return D0; else return D0 + TRI(D0 - 1)
        RTS
        END     START
