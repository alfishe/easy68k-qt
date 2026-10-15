*-----------------------------------------------------------
* Title      : O-13 Graphics Fast Paths (Pixel Loop vs Task 87)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Enable double buffering (task 92, D1 = 17)
            MOVEQ   #92,D0
            MOVEQ   #17,D1
            TRAP    #15

            * 1) Measure naive 16x4 pixel loop (64 TRAP #15 calls) -> D6 cycles
            MOVEQ   #30,D0
            TRAP    #15
            MOVEQ   #10,D2
.YLP:       MOVEQ   #10,D1
.XLP:       MOVEQ   #82,D0
            TRAP    #15
            ADDQ.W  #1,D1
            CMPI.W  #26,D1
            BLT.S   .XLP
            ADDQ.W  #1,D2
            CMPI.W  #14,D2
            BLT.S   .YLP
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D6               ; naive pixel-loop cycles

            * 2) Measure single DrawRectangle (task 87) + Repaint (task 94) -> D7 cycles
            MOVEQ   #30,D0
            TRAP    #15
            MOVEQ   #87,D0
            MOVEQ   #10,D1
            MOVEQ   #10,D2
            MOVEQ   #25,D3
            MOVEQ   #13,D4
            TRAP    #15
            MOVEQ   #94,D0
            TRAP    #15
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D7               ; fast-path cycles (<< D6)

            MOVEQ   #1,D0
            SIMHALT
            END     START
