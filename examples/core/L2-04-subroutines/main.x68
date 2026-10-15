*-----------------------------------------------------------
* Title      : L2-04 Subroutines and the Hardware Stack: BSR, JSR, RTS
* Written by : EASy68K-Qt contributors
* Description: Nested subroutine calls computing 3^2 + 4^2 = 25
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVEQ   #3,D0
        MOVEQ   #4,D1
        BSR.S   SUM_OF_SQUARES  ; returns D0 = 3^2 + 4^2 = 25
        SIMHALT

* Computes D0.L = D0.W^2 + D1.W^2
SUM_OF_SQUARES:
        BSR.S   SQUARE          ; D0 = D0^2 (9)
        MOVE.L  D0,D2           ; save first square in D2
        MOVE.L  D1,D0
        BSR.S   SQUARE          ; D0 = D1^2 (16)
        ADD.L   D2,D0           ; D0 = 9 + 16 = 25
        RTS

* Computes D0.L = D0.W * D0.W
SQUARE:
        MULS.W  D0,D0
        RTS

        END     START
