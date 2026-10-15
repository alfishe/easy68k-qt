*-----------------------------------------------------------
* Title      : L1-04 64-Bit Multiword Arithmetic with ADDX and SUBX
* Written by : EASy68K-Qt contributors
* Description: Add 64-bit integers in D0:D1 and D2:D3 using ADD.L + ADDX.L
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * D0:D1 = $00000001_FFFFFFFF
        MOVEQ   #1,D0
        MOVE.L  #$FFFFFFFF,D1

        * D2:D3 = $00000002_00000002
        MOVEQ   #2,D2
        MOVEQ   #2,D3

        * 64-bit addition: D0:D1 += D2:D3 -> $00000004_00000001
        ADD.L   D3,D1           ; low 32 bits + sets X=1
        ADDX.L  D2,D0           ; high 32 bits + X
        SIMHALT

        END     START
