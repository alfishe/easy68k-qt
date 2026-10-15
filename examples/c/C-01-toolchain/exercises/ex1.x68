*-----------------------------------------------------------
* Exercise C-01-EX1: crt0 .bss clear + main return 123
*-----------------------------------------------------------
            ORG     $1000
_start:
            * TODO: Clear g_zero at $2004 and return g_start + g_zero + 23 in D0
            SIMHALT
            ORG     $2000
g_start:    DC.L    100
g_zero:     DS.L    1
            END     _start
