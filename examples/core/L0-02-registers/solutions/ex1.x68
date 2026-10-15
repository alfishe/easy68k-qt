* L0-02 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #100,D0
        MOVEQ   #-5,D1
        MOVEA.L #$00003000,A1
        SIMHALT
        END     START
