*-----------------------------------------------------------
* Title      : L1-03 Signed vs Unsigned Comparisons
* Written by : EASy68K-Qt contributors
* Description: Compare $FF and $01 as both unsigned (SHI) and signed (SGT)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVEQ   #0,D2
        MOVEQ   #0,D3
        MOVE.B  #$FF,D0         ; 255 unsigned, -1 signed
        MOVE.B  #$01,D1         ; 1 unsigned, +1 signed
        CMP.B   D1,D0           ; compute D0.B - D1.B ($FF - $01 = $FE)
        SHI     D2              ; D2.B = $FF because 255 > 1 (unsigned)
        SGT     D3              ; D3.B = $00 because -1 is NOT > +1 (signed)
        SIMHALT

        END     START
