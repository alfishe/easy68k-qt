* L1-08 Exercise 1: Compute 100 - 37 ($63) using NBCD
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #$37,D0
        * TODO: clear X and negate D0.B with NBCD
        SIMHALT
        END     START
