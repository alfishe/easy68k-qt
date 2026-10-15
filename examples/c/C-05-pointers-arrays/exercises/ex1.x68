*-----------------------------------------------------------
* Exercise C-05-EX1: Reverse 4 words at $2000 in-place
*-----------------------------------------------------------
            ORG     $1000
_start:
            * TODO: Reverse the 4 words at $2000 in-place and return ($2000) in D0
            SIMHALT
            ORG     $2000
            DC.W    10,20,30,40
            END     _start
