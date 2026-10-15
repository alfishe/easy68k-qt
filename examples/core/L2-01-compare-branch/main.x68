*-----------------------------------------------------------
* Title      : L2-01 Compare and Branch: CMP, TST, and Bcc
* Written by : EASy68K-Qt contributors
* Description: Signed max (BGE) vs unsigned max (BHS/BCC) and TST
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * Signed max of D0 = -10 and D1 = 25 -> D2 = 25
        MOVEQ   #-10,D0
        MOVEQ   #25,D1
        MOVE.L  D0,D2
        CMP.L   D1,D2           ; compare D2 against D1
        BGE.S   GOT_SMAX        ; branch if D2 >= D1 (signed)
        MOVE.L  D1,D2
GOT_SMAX:

        * Unsigned max of $FFFF0000 and $00000010 -> D3 = $FFFF0000
        MOVE.L  #$FFFF0000,D0
        MOVEQ   #$10,D1
        MOVE.L  D0,D3
        CMP.L   D1,D3
        BCC.S   GOT_UMAX        ; BCC/BHS: branch if D3 >= D1 (unsigned)
        MOVE.L  D1,D3
GOT_UMAX:

        * TST.L checks sign/zero of D2 -> positive, so set D4 = 1
        MOVEQ   #0,D4
        TST.L   D2
        BLE.S   DONE
        MOVEQ   #1,D4
DONE:
        SIMHALT

        END     START
