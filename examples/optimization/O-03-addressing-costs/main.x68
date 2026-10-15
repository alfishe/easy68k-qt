*-----------------------------------------------------------
* Title      : O-03 Addressing Mode Costs ((An)+ vs d(An,Xi))
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * 1) Naive indexed copy -> cycles in D4
            MOVEQ   #30,D0
            TRAP    #15
            LEA     SRC(PC),A0
            LEA     $2000,A1
            MOVEQ   #0,D1
            MOVEQ   #8-1,D2
.NLP:       MOVE.W  0(A0,D1.W),0(A1,D1.W)
            ADDQ.W  #2,D1
            DBRA    D2,.NLP
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D4               ; naive cycles

            * 2) Optimised post-increment copy -> cycles in D5
            MOVEQ   #30,D0
            TRAP    #15
            LEA     SRC(PC),A0
            LEA     $2000,A1
            MOVEQ   #8-1,D2
.OLP:       MOVE.W  (A0)+,(A1)+
            DBRA    D2,.OLP
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D5               ; optimised cycles (< D4)

            MOVEQ   #0,D0
            MOVE.W  $200E,D0            ; last word = 8
            SIMHALT

SRC:        DC.W    1,2,3,4,5,6,7,8
            END     START
