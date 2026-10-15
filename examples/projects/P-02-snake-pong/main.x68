*-----------------------------------------------------------
* Title      : P-02 Arcade Pong Loop (Double Buffer + 7-Seg + Sound)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
SEG7_DIG0   EQU     $E00000
SEG7_DIG1   EQU     $E00002

            ORG     $1000
START:
            * Enable double-buffered drawing mode (task 92, D1=17)
            MOVEQ   #17,D1
            MOVEQ   #92,D0
            TRAP    #15

            * Ball state: D4 = x (52), D5 = y (30), D6 = vx (+4), D7 = vy (+2)
            MOVEQ   #52,D4
            MOVEQ   #30,D5
            MOVEQ   #4,D6
            MOVEQ   #2,D7
            CLR.L   BOUNCES
            CLR.L   SCORE

            MOVEQ   #4-1,D3             ; Simulate 4 frames
.FRAME_LP:
            * 1. Update physics
            ADD.W   D6,D4               ; x += vx
            ADD.W   D7,D5               ; y += vy
            CMPI.W  #60,D4              ; Hit right paddle at X=60?
            BLT.S   .RENDER
            NEG.W   D6                  ; Reflect vx
            ADDQ.L  #1,BOUNCES
            ADDI.L  #10,SCORE
            * Update 7-segment display with '1' '0' ($06, $3F)
            MOVE.B  #$06,SEG7_DIG0
            MOVE.B  #$3F,SEG7_DIG1

.RENDER:
            * 2. Draw ball pixel at (D4, D5) into offscreen buffer (task 82)
            MOVE.W  D4,D1
            MOVE.W  D5,D2
            MOVEQ   #82,D0
            TRAP    #15

            * 3. Repaint offscreen buffer to screen (task 94)
            MOVEQ   #94,D0
            TRAP    #15

            DBRA    D3,.FRAME_LP

            MOVEQ   #0,D0
            MOVEQ   #0,D1
            MOVE.W  D4,D0               ; Final ball X = 52
            MOVE.W  D5,D1               ; Final ball Y = 38
            MOVE.L  BOUNCES,D2          ; D2 = 1
            MOVE.L  SCORE,D3            ; D3 = 10
            SIMHALT

            ORG     $2000
BOUNCES:    DS.L    1
SCORE:      DS.L    1
            END     START
