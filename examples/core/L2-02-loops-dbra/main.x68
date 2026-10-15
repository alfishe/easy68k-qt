*-----------------------------------------------------------
* Title      : L2-02 Counted Loops with DBRA / DBF and DBcc
* Written by : EASy68K-Qt contributors
* Description: Sum 1..10 using DBRA with N-1 counter initialization
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVEQ   #0,D0           ; accumulator = 0
        MOVEQ   #1,D2           ; current term = 1
        MOVEQ   #10-1,D1        ; 10 iterations -> initialize D1.W to 9
LOOP:
        ADD.L   D2,D0           ; D0 += D2
        ADDQ.L  #1,D2           ; D2++
        DBRA    D1,LOOP         ; decrement D1.W; branch until D1.W == -1
        SIMHALT

        END     START
