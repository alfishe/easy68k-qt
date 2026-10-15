*-----------------------------------------------------------
* Title      : L2-07 Recursion: Factorial and Stack Growth
* Written by : EASy68K-Qt contributors
* Description: Compute 5! = 120 recursively
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVEQ   #5,D0
        BSR.S   FACT            ; D0 = 5! = 120 ($78)
        SIMHALT

* Recursive factorial: input D0.W = n, output D0.L = n!
FACT:
        CMP.W   #1,D0
        BLE.S   .BASE
        MOVE.W  D0,-(SP)        ; save n on the stack
        SUBQ.W  #1,D0           ; n - 1
        BSR.S   FACT            ; D0 = (n - 1)!
        MOVE.W  (SP)+,D1        ; pop saved n
        MULU.W  D1,D0           ; D0 = n * (n - 1)!
        RTS
.BASE:
        MOVEQ   #1,D0
        RTS

        END     START
