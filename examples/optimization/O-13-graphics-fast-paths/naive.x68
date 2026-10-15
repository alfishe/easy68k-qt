*-----------------------------------------------------------
* Title      : O-13 Naive 16x4 Pixel-by-Pixel Fill (64 TRAPs)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #10,D2              ; y = 10..13
.YLP:       MOVEQ   #10,D1              ; x = 10..25
.XLP:       MOVEQ   #82,D0              ; DrawPixel(x, y)
            TRAP    #15
            ADDQ.W  #1,D1
            CMPI.W  #26,D1
            BLT.S   .XLP
            ADDQ.W  #1,D2
            CMPI.W  #14,D2
            BLT.S   .YLP
            SIMHALT
            END     START
