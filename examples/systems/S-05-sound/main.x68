*-----------------------------------------------------------
* Title      : S-05 Sound System
* Description: Standard and multi-voice sound tasks 70-77
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Load WAV into standard player slot 0 (task 71)
            LEA     WAV_NAME(PC),A1
            MOVEQ   #0,D1
            MOVEQ   #71,D0
            TRAP    #15

            * Play standard slot 0 (task 72)
            MOVEQ   #0,D1
            MOVEQ   #72,D0
            TRAP    #15

            * Load WAV into multi-voice slot 1 (task 74)
            LEA     WAV_NAME(PC),A1
            MOVEQ   #1,D1
            MOVEQ   #74,D0
            TRAP    #15

            * Play multi-voice slot 1 (task 75)
            MOVEQ   #1,D1
            MOVEQ   #75,D0
            TRAP    #15

            * Stop all sounds on both players (tasks 76 & 77, D2.L = 3)
            MOVEQ   #0,D1
            MOVEQ   #3,D2
            MOVEQ   #76,D0
            TRAP    #15

            MOVEQ   #1,D1
            MOVEQ   #3,D2
            MOVEQ   #77,D0
            TRAP    #15

            MOVEQ   #1,D3
            SIMHALT

WAV_NAME:   DC.B    'beep.wav',0
            DS.W    0
            END     START
