*-----------------------------------------------------------
* Title      : L1-06 Bitwise Logic and Bit Manipulation
* Written by : EASy68K-Qt contributors
* Description: Demonstrate AND, OR, EOR, BSET, BCLR, BCHG, BTST
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVE.L  #$000000F0,D0
        ANDI.L  #$0000003C,D0   ; D0 = $30

        MOVE.L  #$000000F0,D1
        ORI.L   #$0000000F,D1   ; D1 = $FF

        MOVE.L  #$000000FF,D2
        EORI.L  #$000000AA,D2   ; D2 = $55

        MOVEQ   #0,D3
        BSET    #3,D3           ; D3 = $08
        BSET    #7,D3           ; D3 = $88
        BCHG    #0,D3           ; D3 = $89
        BCLR    #3,D3           ; D3 = $81
        SIMHALT

        END     START
