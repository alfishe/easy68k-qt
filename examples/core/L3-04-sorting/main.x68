*-----------------------------------------------------------
* Title      : L3-04 Sorting in Memory: Insertion Sort
* Written by : EASy68K-Qt contributors
* Description: Sort 6 words at $2000 in ascending order via insertion sort
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * Copy 6 unsorted words to $2000
        LEA     INIT,A0
        LEA     $2000,A1
        MOVEQ   #6-1,D0
.COPY:
        MOVE.W  (A0)+,(A1)+
        DBRA    D0,.COPY

        * Insertion sort on 6 words at $2000
        LEA     $2000,A0        ; base of array
        MOVEQ   #2,D0           ; byte offset i = 2 (second element)
.OUTER:
        MOVE.W  0(A0,D0.W),D1   ; key = arr[i]
        MOVE.W  D0,D2           ; j = i
.INNER:
        TST.W   D2
        BLE.S   .INSERT
        MOVE.W  -2(A0,D2.W),D3  ; arr[j-1]
        CMP.W   D1,D3
        BLE.S   .INSERT         ; if arr[j-1] <= key, stop shifting
        MOVE.W  D3,0(A0,D2.W)   ; arr[j] = arr[j-1]
        SUBQ.W  #2,D2           ; j -= 2
        BRA.S   .INNER
.INSERT:
        MOVE.W  D1,0(A0,D2.W)   ; arr[j] = key
        ADDQ.W  #2,D0
        CMP.W   #12,D0          ; 6 words = 12 bytes
        BLT.S   .OUTER

        MOVEQ   #0,D0
        MOVE.W  $2000,D0        ; smallest element = 4
        SIMHALT

INIT    DC.W    42,15,8,99,23,4

        END     START
