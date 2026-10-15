*-----------------------------------------------------------
* Solution P-03-EX1: Parse hex string "A5" into D0 ($A5) and store at $2000
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     HEX_STR(PC),A0
            MOVEQ   #0,D0
            MOVEQ   #2-1,D1
.LP:        MOVE.B  (A0)+,D2
            CMP.B   #'9',D2
            BLE.S   .DIG
            SUBI.B  #'A'-10,D2
            BRA.S   .ACC
.DIG:       SUBI.B  #'0',D2
.ACC:       ANDI.W  #$0F,D2
            ASL.W   #4,D0
            OR.W    D2,D0
            DBRA    D1,.LP
            MOVE.B  D0,$2000
            SIMHALT
HEX_STR:    DC.B    'A5',0
            END     START
