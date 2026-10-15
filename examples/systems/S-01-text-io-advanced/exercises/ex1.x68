*-----------------------------------------------------------
* Exercise S-01-EX1: Clear screen and set/read cursor
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Clear screen (task 11, D1.W=$FF00), set cursor to
            * col=5, row=2 ($0502), and read cursor back into D0.W
            SIMHALT
            END     START
