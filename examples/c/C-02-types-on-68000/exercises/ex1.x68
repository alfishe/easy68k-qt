*-----------------------------------------------------------
* Exercise C-02-EX1: Sum high and low 16-bit words at $2000
*-----------------------------------------------------------
            ORG     $1000
_start:
            * TODO: Sum the two unsigned 16-bit words at $2000 and $2002 into D0
            SIMHALT
            ORG     $2000
            DC.L    $ABCDEF01
            END     _start
