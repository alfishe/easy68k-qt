* L1-06 Exercise 1: Convert 'A' ($41) in D0 to 'a' ($61) using BSET
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #'A',D0
        * TODO: set bit 5 of D0
        SIMHALT
        END     START
