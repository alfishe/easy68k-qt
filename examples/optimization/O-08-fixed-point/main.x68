*-----------------------------------------------------------
* Title      : O-08 Fixed-Point Arithmetic (Q8.8 & Q16.16 with SWAP)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * 1) Q8.8 multiply: 3.5 ($0380) * 2.25 ($0240) = 7.875 ($07E0)
            MOVE.W  #$0380,D2           ; 3.5 in Q8.8
            MULS.W  #$0240,D2           ; * 2.25 -> $0007E000 (Q16.16)
            ASR.L   #8,D2               ; -> $000007E0 (Q8.8 = 7.875)

            * 2) Q16.16 integer extraction: naive ASR.L #16 vs 4-cycle SWAP
            MOVEQ   #30,D0
            TRAP    #15
            MOVE.L  #$0007E000,D3       ; 7.875 in Q16.16
            MOVEQ   #16,D1
            ASR.L   D1,D3               ; 38-cycle shift
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D4               ; naive cycles (58)

            MOVEQ   #30,D0
            TRAP    #15
            MOVE.L  #$0007E000,D0       ; 7.875 in Q16.16
            SWAP    D0                  ; 4 cycles!
            EXT.L   D0                  ; 4 cycles -> D0 = 7
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D5               ; SWAP cycles (24)

            MOVEQ   #7,D0
            SIMHALT
            END     START
