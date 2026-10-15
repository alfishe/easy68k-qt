*-----------------------------------------------------------
* Title      : S-06 Timing and Frame Pacing
* Description: Task 8 clock read and Task 23 delay
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Read start time in hundredths of a second (task 8)
            MOVEQ   #8,D0
            TRAP    #15
            MOVE.L  D1,D2               ; D2 = start time

            * Delay 25 hundredths of a second (250 ms, task 23)
            MOVEQ   #23,D0
            MOVEQ   #25,D1
            TRAP    #15

            * Read end time (task 8)
            MOVEQ   #8,D0
            TRAP    #15
            MOVE.L  D1,D3               ; D3 = end time

            MOVE.L  D3,D0
            SUB.L   D2,D0               ; D0 = elapsed hundredths (25)
            SIMHALT
            END     START
