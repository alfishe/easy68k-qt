*-----------------------------------------------------------
* Title      : L2-05 Passing Parameters and Saving Registers with MOVEM
* Written by : EASy68K-Qt contributors
* Description: Preserve caller registers D2-D3 with MOVEM.L across a call
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVE.L  #$11111111,D2   ; caller value that must survive call
        MOVE.L  #$22222222,D3   ; caller value that must survive call
        LEA     VEC_A,A0
        LEA     VEC_B,A1
        MOVEQ   #3,D1           ; length = 3 words
        BSR.S   DOT_PRODUCT     ; D0 = 2*5 + 3*6 + 4*7 = 10 + 18 + 28 = 56
        SIMHALT

* Computes dot product of D1.W words at (A0) and (A1) -> result in D0.L
* Preserves D2 and D3 via MOVEM.L
DOT_PRODUCT:
        MOVEM.L D2-D3,-(SP)
        MOVEQ   #0,D0
        SUBQ.W  #1,D1
.LOOP:
        MOVE.W  (A0)+,D2
        MOVE.W  (A1)+,D3
        MULS.W  D3,D2
        ADD.L   D2,D0
        DBRA    D1,.LOOP
        MOVEM.L (SP)+,D2-D3
        RTS

VEC_A   DC.W    2,3,4
VEC_B   DC.W    5,6,7

        END     START
