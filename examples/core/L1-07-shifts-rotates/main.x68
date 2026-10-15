*-----------------------------------------------------------
* Title      : L1-07 Shifts and Rotates: ASL, ASR, LSL, LSR, ROL, ROR
* Written by : EASy68K-Qt contributors
* Description: Multiply by 10 with shifts, compare ASR vs LSR, rotate bytes
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * Multiply D0 = 7 by 10 using (7 << 3) + (7 << 1) = 56 + 14 = 70
        MOVEQ   #7,D0
        MOVE.L  D0,D4
        LSL.L   #3,D0           ; D0 = 56
        LSL.L   #1,D4           ; D4 = 14
        ADD.L   D4,D0           ; D0 = 70 ($46)

        * Arithmetic vs logical right shift on $F0 (-16 signed, 240 unsigned)
        MOVEQ   #0,D1
        MOVE.B  #$F0,D1
        ASR.B   #2,D1           ; D1.B = $FC (-4 signed)

        MOVEQ   #0,D2
        MOVE.B  #$F0,D2
        LSR.B   #2,D2           ; D2.B = $3C (60 unsigned)

        * Rotate byte $81 left by 1 -> $03
        MOVEQ   #0,D3
        MOVE.B  #$81,D3
        ROL.B   #1,D3           ; D3.B = $03
        SIMHALT

        END     START
