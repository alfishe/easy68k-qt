*-----------------------------------------------------------
* Exercise P-03-EX1: Parse hex string "A5" into D0 ($A5) and store at $2000
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Parse HEX_STR ("A5") into D0 = $A5 and write to $2000
            SIMHALT
HEX_STR:    DC.B    'A5',0
            END     START
