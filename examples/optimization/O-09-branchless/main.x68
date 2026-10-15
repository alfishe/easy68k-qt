*-----------------------------------------------------------
* Title      : O-09 Branchless Programming (SUBX & Scc Masks)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Branchless 32-bit abs(-75) using ADD.L + SUBX.L sign mask:
            MOVE.L  #-75,D0
            MOVE.L  D0,D1
            ADD.L   D1,D1               ; X = sign bit of D0 (1 for negative)
            SUBX.L  D1,D1               ; D1 = $FFFFFFFF (-1) if neg, else 0
            EOR.L   D1,D0               ; D0 ^= mask
            SUB.L   D1,D0               ; D0 -= mask -> 75

            * Branchless clamp-to-zero of negative value (-20 -> 0, +35 -> 35)
            MOVEQ   #35,D2
            TST.L   D2
            SPL     D3                  ; D3.B = $FF since D2 >= 0
            EXT.W   D3
            EXT.L   D3                  ; D3.L = $FFFFFFFF
            AND.L   D3,D2               ; D2 = 35

            SIMHALT
            END     START
