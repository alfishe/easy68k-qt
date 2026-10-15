*-----------------------------------------------------------
* Exercise S-07-EX1: Display '01' on the 7-segment display
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Query 7-seg base address (task 32, D1.B=1) and write
            * $3F ('0') to 0(A0) and $06 ('1') to 2(A0)
            SIMHALT
            END     START
