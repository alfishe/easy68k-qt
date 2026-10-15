*-----------------------------------------------------------
* Title      : P-06 C++17 Game Framework + Hand-Tuned ASM Hot Path
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     $010000,A7
            BSR.S   MAIN
            SIMHALT

*--- Hand-tuned ASM hot path: step N entities (x += vx) ---
* A0 = &entities[0], D0.W = N - 1
STEP_ENTITIES_ASM:
.LP:        MOVE.W  2(A0),D1            ; vx
            ADD.W   D1,(A0)             ; x += vx
            ADDQ.L  #4,A0               ; next Entity (sizeof == 4)
            DBRA    D0,.LP
            RTS

*--- Hand-tuned ASM hot path: 1D AABB overlap (|x0 - x1| < width) ---
* D0.W = x0, D1.W = x1, D2.W = width -> D0.L = 1 if overlap else 0
CHECK_AABB_ASM:
            SUB.W   D1,D0               ; diff = x0 - x1
            BPL.S   .POS
            NEG.W   D0                  ; |diff|
.POS:       CMP.W   D2,D0
            BLT.S   .HIT
            MOVEQ   #0,D0
            RTS
.HIT:       MOVEQ   #1,D0
            RTS

MAIN:
            * Step world for 4 ticks
            MOVEQ   #4-1,D3
.TICK_LP:
            LEA     WORLD_ENTITIES,A0
            MOVEQ   #2-1,D0
            BSR.S   STEP_ENTITIES_ASM
            DBRA    D3,.TICK_LP

            * Check AABB collision between entity 0 and entity 1 (width = 8)
            LEA     WORLD_ENTITIES,A0
            MOVE.W  0(A0),D0            ; x0 = 60
            MOVE.W  4(A0),D1            ; x1 = 60
            MOVEQ   #8,D2
            BSR.S   CHECK_AABB_ASM
            MOVE.L  D0,D4               ; D4 = hits (1)

            * Print "Tick 4: x0=60 hits=1\r\n"
            LEA     MSG1(PC),A1
            MOVEQ   #14,D0
            TRAP    #15

            MOVEQ   #0,D1
            MOVE.W  WORLD_ENTITIES,D1   ; x0 = 60
            MOVEQ   #3,D0
            TRAP    #15

            LEA     MSG2(PC),A1
            MOVEQ   #14,D0
            TRAP    #15

            MOVE.L  D4,D1               ; hits = 1
            MOVEQ   #3,D0
            TRAP    #15

            LEA     CRLF(PC),A1
            MOVEQ   #14,D0
            TRAP    #15

            MOVEQ   #0,D0
            MOVE.W  WORLD_ENTITIES,D0
            ADD.L   D4,D0               ; return x0 + hits = 61
            RTS

            ORG     $2000
WORLD_ENTITIES:
            DC.W    40, 5               ; Entity 0: x=40, vx=+5
            DC.W    64, -1              ; Entity 1: x=64, vx=-1
MSG1:       DC.B    'Tick 4: x0=',0
MSG2:       DC.B    ' hits=',0
CRLF:       DC.B    13,10,0
            END     START
