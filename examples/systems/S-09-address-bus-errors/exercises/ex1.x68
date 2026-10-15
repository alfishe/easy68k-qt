*-----------------------------------------------------------
* Exercise S-09-EX1: Recover from odd longword access at $3003
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            * TODO: Install Vector 3 ($000C) handler that pops the faulting
            * address into D0 and recovers via RTE; trigger with MOVE.L (A1),D1
            * where A1 = $3003
            SIMHALT
            END     START
