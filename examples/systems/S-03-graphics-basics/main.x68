*-----------------------------------------------------------
* Title      : S-03 Graphics Basics
* Description: Pixels, lines, rectangles, and pen queries
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Set pen colour to red ($000000FF) and fill colour to blue ($00FF0000)
            MOVEQ   #80,D0
            MOVE.L  #$000000FF,D1
            TRAP    #15
            MOVEQ   #81,D0
            MOVE.L  #$00FF0000,D1
            TRAP    #15

            * Draw pixel at (10, 20) and read it back via task 83
            MOVEQ   #82,D0
            MOVEQ   #10,D1
            MOVEQ   #20,D2
            TRAP    #15

            MOVEQ   #83,D0
            MOVEQ   #10,D1
            MOVEQ   #20,D2
            TRAP    #15
            MOVE.L  D0,D5               ; D5 = $000000FF

            * Draw line from (0, 0) to (100, 50) via task 84
            MOVEQ   #84,D0
            MOVEQ   #0,D1
            MOVEQ   #0,D2
            MOVEQ   #100,D3
            MOVEQ   #50,D4
            TRAP    #15

            * Query pen position via task 96 -> D1=100, D2=50
            MOVEQ   #96,D0
            TRAP    #15
            MOVE.L  D1,D6
            MOVE.L  D2,D7

            * Draw filled rectangle (10, 10) to (30, 30) via task 87
            MOVEQ   #87,D0
            MOVEQ   #10,D1
            MOVEQ   #10,D2
            MOVEQ   #30,D3
            MOVEQ   #30,D4
            TRAP    #15

            SIMHALT
            END     START
