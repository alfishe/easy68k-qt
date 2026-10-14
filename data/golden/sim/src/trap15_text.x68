* Trap #15 text and number tasks (Task 10.4), compared with the original
* Sim68K core by tools/reference/compare_sim.sh; keyboard input comes from
* trap15_text.in. CC0-1.0.
START   ORG     $1000
        LEA     MSG1,A1
        MOVE.W  #5,D1
        MOVEQ   #0,D0           ; 0: D1.W chars with CRLF
        TRAP    #15
        MOVE.W  #3,D1
        MOVEQ   #1,D0           ; 1: D1.W chars, no CRLF
        TRAP    #15
        MOVE.W  #0,D1
        MOVEQ   #1,D0           ; zero length
        TRAP    #15
        MOVE.L  #-123456,D1
        MOVEQ   #3,D0           ; 3: signed decimal
        TRAP    #15
        MOVE.L  #$7FFFFFFF,D1
        TRAP    #15
        MOVE.L  #$80000000,D1
        TRAP    #15
        MOVE.B  #'!',D1
        MOVEQ   #6,D0           ; 6: char
        TRAP    #15
        LEA     MSG2,A1
        MOVEQ   #13,D0          ; 13: NUL string with CRLF
        TRAP    #15
        MOVEQ   #14,D0          ; 14: NUL string, no CRLF
        TRAP    #15
        MOVE.L  #$DEADBEEF,D1
        MOVE.B  #16,D2
        MOVEQ   #15,D0          ; 15: unsigned in radix D2
        TRAP    #15
        MOVE.B  #2,D2
        MOVE.L  #10,D1
        TRAP    #15
        MOVE.B  #10,D2
        MOVE.L  #$FFFFFFFF,D1
        TRAP    #15
        MOVE.B  #36,D2
        MOVE.L  #123456789,D1
        TRAP    #15
        LEA     MSG3,A1
        MOVE.L  #42,D1
        MOVEQ   #17,D0          ; 17: string then number
        TRAP    #15
        MOVE.L  #-7,D1
        MOVE.B  #8,D2
        MOVEQ   #20,D0          ; 20: number right-justified in D2.B
        TRAP    #15
        MOVE.L  #12345,D1
        MOVE.B  #3,D2
        TRAP    #15
* input
        LEA     BUF,A1
        MOVEQ   #2,D0           ; 2: read string to (A1), length in D1
        TRAP    #15
        MOVEQ   #13,D0
        TRAP    #15
        MOVE.L  D1,D3
        MOVEQ   #4,D0           ; 4: read number to D1
        TRAP    #15
        MOVE.L  D1,D4
        MOVEQ   #5,D0           ; 5: read char to D1.B
        TRAP    #15
        MOVE.B  D1,D5
        MOVEQ   #12,D0          ; 12: echo off
        MOVEQ   #0,D1
        TRAP    #15
        LEA     MSG4,A1
        MOVEQ   #18,D0          ; 18: prompt string, read number
        TRAP    #15
        MOVE.L  D1,D6
        MOVEQ   #12,D0          ; echo on
        MOVEQ   #1,D1
        TRAP    #15
        MOVEQ   #4,D0
        TRAP    #15             ; non-numeric input
        MOVE.L  D1,D7
        MOVE.L  #$12345678,D1
        MOVEQ   #31,D0          ; 31: cycles to D1:D0
        TRAP    #15
        MOVE.L  D1,A2
        MOVE.L  D0,A3
        MOVEQ   #30,D0          ; 30: clear cycle counter
        TRAP    #15
        MOVEQ   #9,D0           ; 9: halt
        TRAP    #15
        NOP
MSG1    DC.B    'HelloWorld'
MSG2    DC.B    'NUL terminated',0
MSG3    DC.B    'answer=',0
MSG4    DC.B    'number? ',0
BUF     DS.B    82
        END     START
