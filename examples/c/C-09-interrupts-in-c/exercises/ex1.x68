*-----------------------------------------------------------
* Exercise C-09-EX1: TRAP #0 C handler + critical section -> 150
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            * TODO: Install trap0_trampoline at $0080, invoke TRAP #0,
            * add 50 to g_shared ($2000) in a critical section, and load into D0
            SIMHALT
            ORG     $2000
g_shared:   DS.L    1
            END     _start
