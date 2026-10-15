*-----------------------------------------------------------
* Exercise P-04-EX1: Implement W_OVER ( a b -- a b a ) and compute 6 10 OVER * + = 66
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Run Forth stream LIT 6, LIT 10, OVER, MUL, ADD, HALT -> D0 = 66
            SIMHALT
            END     START
