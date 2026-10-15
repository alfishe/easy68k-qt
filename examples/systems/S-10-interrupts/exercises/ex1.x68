*-----------------------------------------------------------
* Exercise S-10-EX1: Level 4 autovector ISR at $0070
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            * TODO: Install Level 4 ISR at $0070 that sets D0 = 42, enable
            * auto-IRQ Level 4 (D2.B=$84), execute STOP #$2000, then disable
            * auto-IRQ (D2.B=0) before SIMHALT
            SIMHALT
            END     START
