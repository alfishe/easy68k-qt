* L1-07 Exercise 1: Swap nibbles of $3C in D0.B using ROL.B #4,D0
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #$3C,D0
        * TODO: rotate D0.B by 4 bits so D0 = $C3
        SIMHALT
        END     START
