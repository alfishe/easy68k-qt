*-----------------------------------------------------------
* Title      : L3-02 1-D and 2-D Arrays
* Written by : EASy68K-Qt contributors
* Description: Sum a 1-D array with (A0)+ and index a 3x4 word matrix
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * 1. Sum 5 longwords in ARR1D -> D0 = 10+20+30+40+50 = 150
        LEA     ARR1D,A0
        MOVEQ   #0,D0
        MOVEQ   #5-1,D1
.SUM:
        ADD.L   (A0)+,D0
        DBRA    D1,.SUM

        * 2. Read MAT[row=2][col=1] from a 3x4 word matrix -> D2 = 99
        LEA     MAT,A0
        MOVEQ   #2,D1           ; row = 2
        MOVEQ   #1,D3           ; col = 1
        LSL.W   #2,D1           ; row * 4 (COLS = 4)
        ADD.W   D3,D1           ; row * 4 + col = 9
        ADD.W   D1,D1           ; byte offset = 9 * 2 = 18
        MOVEQ   #0,D2
        MOVE.W  0(A0,D1.W),D2   ; D2 = 99
        SIMHALT

ARR1D   DC.L    10,20,30,40,50

MAT     DC.W    11,12,13,14     ; row 0
        DC.W    21,22,23,24     ; row 1
        DC.W    31,99,33,34     ; row 2 (col 1 is 99)

        END     START
