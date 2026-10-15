*-----------------------------------------------------------
* Title      : L2-06 Stack Frames with LINK and UNLK
* Written by : EASy68K-Qt contributors
* Description: Compute 2*(width + height) using stack args and LINK/UNLK
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVE.L  #$ABCD1234,A6   ; sentinel to prove A6 is preserved
        MOVE.L  #15,-(SP)       ; push height (second arg, at 12(A6))
        MOVE.L  #10,-(SP)       ; push width  (first arg,  at 8(A6))
        BSR.S   RECT_PERIM      ; D0 = 2 * (10 + 15) = 50
        ADDQ.L  #8,SP           ; caller cleans 2 longword args
        SIMHALT

RECT_PERIM:
        LINK    A6,#-4          ; allocate 4-byte local at -4(A6)
        MOVE.L  8(A6),D0        ; width
        ADD.L   12(A6),D0       ; width + height
        MOVE.L  D0,-4(A6)       ; store half-perimeter in local variable
        MOVE.L  -4(A6),D0
        ADD.L   D0,D0           ; perimeter = 2 * half-perimeter
        UNLK    A6
        RTS

        END     START
