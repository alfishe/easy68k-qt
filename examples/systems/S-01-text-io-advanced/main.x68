*-----------------------------------------------------------
* Title      : S-01 Advanced Text I/O
* Description: Cursor, colours, echo, prompt, and key state
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Position cursor at col=10 ($0A), row=4 ($04)
            MOVEQ   #11,D0
            MOVE.W  #$0A04,D1
            TRAP    #15

            * Read back cursor position into D4
            MOVEQ   #11,D0
            MOVE.W  #$00FF,D1
            TRAP    #15
            MOVEQ   #0,D4
            MOVE.W  D1,D4               ; D4 = $0A04

            * Set font colour green ($0000FF00), Courier 10pt bold
            MOVEQ   #21,D0
            MOVE.L  #$0000FF00,D1
            MOVE.L  #$010A0001,D2
            TRAP    #15

            * Disable keyboard echo (task 12) and input prompt (task 16)
            MOVEQ   #12,D0
            MOVEQ   #0,D1
            TRAP    #15
            MOVEQ   #16,D0
            MOVEQ   #0,D1
            TRAP    #15

            * Query key state for 'A' ($41) via task 19
            MOVEQ   #19,D0
            MOVE.L  #$00000041,D1
            TRAP    #15
            MOVE.L  D1,D3               ; 0 when no key is held

            LEA     MSG(PC),A1
            MOVEQ   #13,D0
            TRAP    #15

            SIMHALT

MSG:        DC.B    'Cursor=0A04',0
            DS.W    0
            END     START
