*-----------------------------------------------------------
* Exercise C-04-EX1: Dot product of A and B -> 70 in D0
*-----------------------------------------------------------
            ORG     $1000
_start:
            * TODO: Compute dot product of 4 words at VEC_A and VEC_B into D0
            SIMHALT
VEC_A:      DC.W    1,2,3,4
VEC_B:      DC.W    5,6,7,8
            END     _start
