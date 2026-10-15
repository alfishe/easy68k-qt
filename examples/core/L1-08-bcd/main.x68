*-----------------------------------------------------------
* Title      : L1-08 Binary-Coded Decimal (BCD): ABCD, SBCD, NBCD
* Written by : EASy68K-Qt contributors
* Description: Packed BCD addition and subtraction with ABCD and SBCD
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * 28 + 35 = 63 ($63 in BCD)
        MOVEQ   #$28,D0
        MOVEQ   #$35,D1
        MOVE    #0,CCR          ; clear X before ABCD
        ABCD    D1,D0           ; D0.B = $63

        * 72 - 19 = 53 ($53 in BCD)
        MOVEQ   #$72,D2
        MOVEQ   #$19,D3
        MOVE    #0,CCR          ; clear X before SBCD
        SBCD    D3,D2           ; D2.B = $53
        SIMHALT

        END     START
