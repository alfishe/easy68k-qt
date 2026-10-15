* L2-01 Exercise 1: Compute D0 = abs(D0) for D0 = -42
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #-42,D0
        * TODO: if D0 < 0, negate D0
        SIMHALT
        END     START
