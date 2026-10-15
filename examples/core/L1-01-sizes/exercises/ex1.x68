* L1-01 Exercise 1: Sign-extend byte $F6 (-10) in D0.B to 32-bit D0.L
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVE.L  #$AABBCCF6,D0   ; upper bytes contain garbage, low byte is -10
        * TODO: sign-extend D0.B to all 32 bits of D0
        SIMHALT
        END     START
