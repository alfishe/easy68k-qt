*-----------------------------------------------------------
* Solution S-02-EX1: Round-trip $CAFEBABE through a file
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     FNAME(PC),A1
            MOVEQ   #52,D0
            TRAP    #15
            MOVE.L  D1,D4

            MOVE.L  D4,D1
            LEA     DATA(PC),A1
            MOVEQ   #4,D2
            MOVEQ   #54,D0
            TRAP    #15

            MOVE.L  D4,D1
            MOVEQ   #0,D2
            MOVEQ   #55,D0
            TRAP    #15

            MOVE.L  D4,D1
            LEA     $2000,A1
            MOVEQ   #4,D2
            MOVEQ   #53,D0
            TRAP    #15

            MOVE.L  D4,D1
            MOVEQ   #56,D0
            TRAP    #15

            LEA     FNAME(PC),A1
            MOVEQ   #57,D0
            TRAP    #15

            MOVE.L  $2000,D0
            SIMHALT

FNAME:      DC.B    'ex_tmp.bin',0
            DS.W    0
DATA:       DC.L    $CAFEBABE
            END     START
