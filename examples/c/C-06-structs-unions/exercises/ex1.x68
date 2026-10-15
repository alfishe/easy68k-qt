*-----------------------------------------------------------
* Exercise C-06-EX1: Read SPR_X (2(A0)) + SPR_Y (4(A0)) into D0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $2000,A0
            * TODO: Sum word at 2(A0) and word at 4(A0) into D0
            SIMHALT
            ORG     $2000
            DC.B    7,0
            DC.W    120,80
            DC.L    $00FF8000
            END     _start
