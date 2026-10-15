*-----------------------------------------------------------
* Solution S-13-EX1: Configure Level 3 Keyboard IRQ (task 62)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     ISR(PC),A0
            MOVE.L  A0,$006C
            MOVEQ   #62,D0
            MOVE.W  #$0301,D1
            TRAP    #15
            MOVEQ   #62,D0
            MOVEQ   #0,D1
            TRAP    #15
            MOVEQ   #62,D0
            SIMHALT
ISR:        RTE
            END     START
