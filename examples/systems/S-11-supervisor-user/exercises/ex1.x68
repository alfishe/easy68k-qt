*-----------------------------------------------------------
* Exercise S-11-EX1: Read user stack from supervisor TRAP #0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            * TODO: Set USP=$00FE0000, install TRAP #0 at $0080, drop to user
            * mode, push $12345678 on A7, and invoke TRAP #0 to load (USP) into D0
            SIMHALT
            END     START
