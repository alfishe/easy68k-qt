*-----------------------------------------------------------
* Title      : L1-05 Multiply and Divide: MULU, MULS, DIVU, DIVS
* Written by : EASy68K-Qt contributors
* Description: Demonstrate MULU, MULS, DIVU, and unpacking remainder via SWAP
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * Unsigned multiply: 300 * 400 = 120000 ($0001D4C0)
        MOVE.W  #300,D0
        MULU.W  #400,D0

        * Signed multiply: -20 * 15 = -300 ($FFFFFED4)
        MOVE.W  #-20,D1
        MULS.W  #15,D1

        * Unsigned divide: 100 / 7 = 14 ($000E) remainder 2 ($0002)
        MOVEQ   #100,D2
        DIVU.W  #7,D2           ; D2 = $0002000E
        MOVE.L  D2,D3
        ANDI.L  #$0000FFFF,D2   ; D2 = quotient (14)
        SWAP    D3
        ANDI.L  #$0000FFFF,D3   ; D3 = remainder (2)
        SIMHALT

        END     START
