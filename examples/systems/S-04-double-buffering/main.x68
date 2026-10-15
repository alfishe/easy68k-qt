*-----------------------------------------------------------
* Title      : S-04 Double Buffering
* Description: Flicker-free frame loop with tasks 92 and 94
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Enable double buffering (task 92, D1.B = 17)
            MOVEQ   #92,D0
            MOVEQ   #17,D1
            TRAP    #15

            MOVEQ   #0,D3               ; frame counter
            MOVEQ   #0,D4               ; current X
            MOVEQ   #4-1,D5             ; 4 frames
FRAME_LOOP:
            MOVE.L  D3,D4
            LSL.L   #4,D4               ; X = frame * 16

            * Draw box at (X, 20) .. (X+12, 32)
            MOVEQ   #87,D0
            MOVE.W  D4,D1
            MOVEQ   #20,D2
            MOVE.W  D4,D3
            ADD.W   #12,D3
            MOVEQ   #32,D4
            TRAP    #15

            * Present offscreen buffer (task 94)
            MOVEQ   #94,D0
            TRAP    #15

            * Recompute D4 = frame * 16 and advance frame counter
            MOVE.L  D5,D0               ; keep loop register safe
            MOVEQ   #4-1,D4
            SUB.L   D5,D4               ; frame index 0..3
            LSL.L   #4,D4               ; 0, 16, 32, 48
            DBRA    D5,FRAME_LOOP

            * Disable double buffering (task 92, D1.B = 16)
            MOVEQ   #92,D0
            MOVEQ   #16,D1
            TRAP    #15

            MOVEQ   #4,D3               ; 4 frames completed
            SIMHALT
            END     START
