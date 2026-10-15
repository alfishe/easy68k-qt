*-----------------------------------------------------------
* Title      : L3-05 Searching: Linear Search vs Binary Search
* Written by : EASy68K-Qt contributors
* Description: Binary search for 47 in an 8-word sorted array -> index 5
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        LEA     SORTED,A0
        MOVEQ   #47,D1          ; target value = 47
        MOVEQ   #0,D2           ; lo = 0
        MOVEQ   #7,D3           ; hi = 7
        MOVEQ   #-1,D0          ; default: not found (-1)

.LOOP:
        CMP.W   D3,D2
        BGT.S   .DONE           ; if lo > hi, not found
        MOVE.W  D2,D4
        ADD.W   D3,D4
        ASR.W   #1,D4           ; mid = (lo + hi) >> 1
        MOVE.W  D4,D5
        ADD.W   D5,D5           ; byte offset = mid * 2
        MOVE.W  0(A0,D5.W),D6   ; probe = SORTED[mid]
        CMP.W   D1,D6
        BEQ.S   .FOUND
        BGT.S   .LEFT
        MOVE.W  D4,D2
        ADDQ.W  #1,D2           ; lo = mid + 1
        BRA.S   .LOOP
.LEFT:
        MOVE.W  D4,D3
        SUBQ.W  #1,D3           ; hi = mid - 1
        BRA.S   .LOOP
.FOUND:
        MOVEQ   #0,D0
        MOVE.W  D4,D0           ; return index 5
.DONE:
        SIMHALT

SORTED  DC.W    5,12,19,26,33,47,58,71

        END     START
