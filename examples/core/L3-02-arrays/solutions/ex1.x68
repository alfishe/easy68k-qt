* L3-02 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     DATA,A0
        MOVEQ   #0,D0
        MOVE.W  (A0)+,D0
        MOVEQ   #4-1,D1
.LOOP:
        MOVE.W  (A0)+,D2
        CMP.W   D0,D2
        BLE.S   .NEXT
        MOVE.W  D2,D0
.NEXT:
        DBRA    D1,.LOOP
        SIMHALT

DATA    DC.W    14,52,8,91,37
        END     START
