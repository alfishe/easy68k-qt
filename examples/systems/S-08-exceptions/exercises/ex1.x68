*-----------------------------------------------------------
* Exercise S-08-EX1: TRAP #1 handler that doubles D0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            * TODO: Install TRAP #1 handler at $0084 (Vector 33) that doubles D0,
            * then set D0 = 15 and invoke TRAP #1 twice
            SIMHALT
            END     START
