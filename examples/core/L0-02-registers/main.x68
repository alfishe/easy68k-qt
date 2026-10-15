*-----------------------------------------------------------
* Title      : L0-02 Loading Data and Address Registers
* Written by : EASy68K-Qt contributors
* Description: Load D0-D3 and A0 using MOVEQ, MOVE.L, and MOVEA.L
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVEQ   #42,D0          ; D0 = $0000002A (4 cycles)
        MOVEQ   #-1,D1          ; D1 = $FFFFFFFF (sign-extended)
        MOVE.L  #$12345678,D2   ; D2 = $12345678 (12 cycles)
        MOVE.L  D0,D3           ; copy D0 into D3 ($0000002A)
        MOVEA.L #$00002000,A0   ; A0 = $00002000
        SIMHALT

        END     START
