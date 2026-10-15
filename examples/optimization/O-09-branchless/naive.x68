*-----------------------------------------------------------
* Title      : O-09 Branching abs(x)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVE.L  #-75,D0
            TST.L   D0
            BPL.S   .POS
            NEG.L   D0
.POS:       SIMHALT
            END     START
