*-----------------------------------------------------------
* Exercise S-02-EX1: Round-trip $CAFEBABE through a file
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Create ex_tmp.bin, write 4 bytes from DATA, seek 0,
            * read into $2000, close, delete, and load ($2000) into D0
            SIMHALT

FNAME:      DC.B    'ex_tmp.bin',0
            DS.W    0
DATA:       DC.L    $CAFEBABE
            END     START
