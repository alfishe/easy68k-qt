*-----------------------------------------------------------
* Title      : S-07 Hardware Window
* Description: 7-segment display, LEDs, switches, buttons
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Show hardware window (task 32, D1.B = 0)
            MOVEQ   #32,D0
            MOVEQ   #0,D1
            TRAP    #15

            * Get 7-segment display address (subtask 1 -> $E00000)
            MOVEQ   #32,D0
            MOVEQ   #1,D1
            TRAP    #15
            MOVEA.L D1,A0

            * Write '6' ($7D) and '8' ($7F) to first two 7-seg digits
            MOVE.B  #$7D,0(A0)
            MOVE.B  #$7F,2(A0)

            * Get LED address (subtask 2 -> $E00010) and write $A5
            MOVEQ   #32,D0
            MOVEQ   #2,D1
            TRAP    #15
            MOVEA.L D1,A1
            MOVE.B  #$A5,(A1)

            * Get switch address (subtask 3) and button address (subtask 7)
            MOVEQ   #32,D0
            MOVEQ   #3,D1
            TRAP    #15
            MOVE.L  D1,D4               ; $E00012

            MOVEQ   #32,D0
            MOVEQ   #7,D1
            TRAP    #15
            MOVE.L  D1,D5               ; $E00014

            MOVEQ   #0,D0
            MOVE.B  (A1),D0             ; read back LED byte ($A5)
            SIMHALT
            END     START
