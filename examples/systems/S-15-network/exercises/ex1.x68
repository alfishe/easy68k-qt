*-----------------------------------------------------------
* Exercise S-15-EX1: Pack TCP port 4096 ($1000) parameter word
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Pack port $1000 in upper word and TCP (1) in low word of D0
            SIMHALT
            END     START
