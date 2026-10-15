*-----------------------------------------------------------
* Exercise S-12-EX1: Catch write to MEMORY READ range at $2100
*-----------------------------------------------------------
            MEMORY  READ $2100,$21FF
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            * TODO: Install Vector 2 ($0008) handler that pops faulting address
            * into D0 and recovers via RTE; trigger with MOVE.W D1,(A0) at A0=$2100
            SIMHALT
            END     START
