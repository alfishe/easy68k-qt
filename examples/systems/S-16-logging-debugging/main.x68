*-----------------------------------------------------------
* Title      : S-16 Logging and Advanced Debugging
* Description: Cycle counter (tasks 30/31) and debug markers
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Clear cycle counter (task 30)
            MOVEQ   #30,D0
            TRAP    #15

            * Measured section: 4 + 8 = 12 cycles (+ 4 for MOVEQ #31,D0 = 16)
            MOVEQ   #42,D2              ; 4 cycles
            ADDQ.L  #8,D2               ; 8 cycles -> D2 = 50

            * Read cycle counter into D1.L (task 31)
            MOVEQ   #31,D0              ; 4 cycles
            TRAP    #15
            MOVE.L  D1,D3               ; D3 = 16 cycles

            * Write to watched memory location $2000 for Advanced Breakpoints
            MOVE.L  D2,$2000
            SIMHALT
            END     START
