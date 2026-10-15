*-----------------------------------------------------------
* Solution S-05-EX1: Load slot 2 and stop-all via task 76
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     WAV_NAME(PC),A1
            MOVEQ   #2,D1
            MOVEQ   #71,D0
            TRAP    #15

            MOVEQ   #2,D1
            MOVEQ   #3,D2
            MOVEQ   #76,D0
            TRAP    #15

            MOVEQ   #76,D0
            SIMHALT
WAV_NAME:   DC.B    'tone.wav',0
            DS.W    0
            END     START
