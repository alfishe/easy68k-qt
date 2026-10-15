*-----------------------------------------------------------
* Title      : O-07 Lookup Tables (Bit Loop vs Nybble Table)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * 1) Naive 8-bit loop popcount($D5) -> D4 cycles
            MOVEQ   #30,D0
            TRAP    #15
            MOVE.B  #$D5,D1
            MOVEQ   #0,D2
            MOVEQ   #8-1,D6
.NLP:       LSR.B   #1,D1
            BCC.S   .NSKIP
            ADDQ.L  #1,D2
.NSKIP:     DBRA    D6,.NLP
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D4               ; naive cycles

            * 2) Nybble lookup table popcount($D5) -> D5 cycles
            MOVEQ   #30,D0
            TRAP    #15
            LEA     POP4(PC),A0
            MOVEQ   #0,D1
            MOVE.B  #$D5,D1
            MOVE.W  D1,D6
            ANDI.W  #$000F,D1           ; low nybble ($05)
            LSR.W   #4,D6               ; high nybble ($0D)
            MOVEQ   #0,D3
            MOVE.B  0(A0,D1.W),D3
            ADD.B   0(A0,D6.W),D3       ; D3 = 2 + 3 = 5
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D5               ; table cycles (< D4)

            MOVE.L  D3,D0               ; D0 = 5
            SIMHALT

POP4:       DC.B    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4
            DS.W    0
            END     START
