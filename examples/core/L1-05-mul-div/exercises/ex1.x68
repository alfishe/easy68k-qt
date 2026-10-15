* L1-05 Exercise 1: Compute D0 = 50 / 6 and D1 = 50 % 6
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #50,D0
        * TODO: divide by 6, leave quotient in D0.L and remainder in D1.L
        SIMHALT
        END     START
