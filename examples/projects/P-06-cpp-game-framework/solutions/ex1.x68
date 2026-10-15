*-----------------------------------------------------------
* Solution P-06-EX1: Step 2 entities 1 tick (x0=24, x1=26) and check AABB (D0=1, D1=50)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #20+4,D2            ; x0 = 24
            MOVEQ   #28-2,D3            ; x1 = 26
            MOVE.L  D2,D1
            ADD.L   D3,D1               ; D1 = x0 + x1 = 50
            MOVE.W  D2,D4
            SUB.W   D3,D4
            BPL.S   .POS
            NEG.W   D4
.POS:       CMPI.W  #8,D4
            SLT     D0
            NEG.B   D0
            ANDI.L  #1,D0               ; D0 = 1
            SIMHALT
            END     START
