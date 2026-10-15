*-----------------------------------------------------------
* Title      : P-07 Fixed-Point Sine Plasma Demo Effect
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     SIN16(PC),A0
            LEA     PLASMA_BUF,A1
            MOVEQ   #0,D0               ; D0 = total intensity checksum
            MOVEQ   #0,D3               ; y = 0

.ROW_LP:
            * Hoist Y sine lookup outside inner X loop!
            MOVE.W  D3,D4
            ADDQ.W  #2,D4               ; phase_y = 2
            ANDI.W  #$0F,D4
            MOVEQ   #0,D5
            MOVE.B  0(A0,D4.W),D5       ; D5 = SIN16[(y + 2) & 15]

            MOVEQ   #0,D6               ; x = 0
.COL_LP:
            MOVE.W  D6,D4
            ANDI.W  #$0F,D4
            MOVEQ   #0,D7
            MOVE.B  0(A0,D4.W),D7       ; D7 = SIN16[x & 15]
            ADD.W   D5,D7               ; intensity = sin_x + sin_y
            MOVE.B  D7,(A1)+            ; write pixel to tile buffer
            ADD.L   D7,D0               ; accumulate checksum
            ADDQ.W  #1,D6
            CMPI.W  #8,D6
            BLT.S   .COL_LP

            ADDQ.W  #1,D3
            CMPI.W  #8,D3
            BLT.S   .ROW_LP

            MOVEQ   #0,D1
            MOVE.B  PLASMA_BUF,D1       ; D1 = pixel (0,0) = 64 + 110 = 174
            MOVEQ   #0,D2
            MOVE.B  PLASMA_BUF+36,D2    ; D2 = pixel (4,4) = 128 + 110 = 238
            SIMHALT

* 16-entry unsigned sine wave (centered at 64, range 0..128, sum of 0..7 = 736)
SIN16:      DC.B    64,88,110,123,128,123,110,88
            DC.B    64,40,18,5,0,5,18,40

            ORG     $2000
PLASMA_BUF: DS.B    64
            END     START
