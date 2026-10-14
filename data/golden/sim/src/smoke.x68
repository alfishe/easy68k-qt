* Smoke test for the golden simulation traces (Task 10.1): text output,
* arithmetic with flags, a stack push and SIMHALT. CC0-1.0.
START   ORG     $1000
        LEA     MSG,A1
        MOVE.B  #14,D0
        TRAP    #15
        MOVE.L  #$12345678,D1
        ADD.L   D1,D1
        MOVE.W  #-1,D2
        MULS    D2,D2
        DIVU    #3,D1
        MOVE.L  D1,-(SP)
        MOVEQ   #3,D0
        TRAP    #15
        SIMHALT
MSG     DC.B    'Hello',13,10,0
        END     START
