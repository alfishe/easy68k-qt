* L2-01 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #-42,D0
        TST.L   D0
        BPL.S   POS
        NEG.L   D0
POS:
        SIMHALT
        END     START
