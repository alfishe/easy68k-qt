*-----------------------------------------------------------
* Title      : S-02 File I/O
* Description: Create, write, seek, read, close, and delete
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Create new file (task 52) -> FID in D1.L
            LEA     FNAME(PC),A1
            MOVEQ   #52,D0
            TRAP    #15
            MOVE.L  D1,D4               ; save FID in D4

            * Write 4 bytes from PAYLOAD (task 54)
            MOVE.L  D4,D1
            LEA     PAYLOAD(PC),A1
            MOVEQ   #4,D2
            MOVEQ   #54,D0
            TRAP    #15

            * Seek back to byte 0 (task 55)
            MOVE.L  D4,D1
            MOVEQ   #0,D2
            MOVEQ   #55,D0
            TRAP    #15

            * Read 4 bytes into $2000 (task 53)
            MOVE.L  D4,D1
            LEA     $2000,A1
            MOVEQ   #4,D2
            MOVEQ   #53,D0
            TRAP    #15

            * Close file (task 56)
            MOVE.L  D4,D1
            MOVEQ   #56,D0
            TRAP    #15

            * Delete temporary file (task 57)
            LEA     FNAME(PC),A1
            MOVEQ   #57,D0
            TRAP    #15

            MOVE.L  $2000,D3            ; $36384B21 ('68K!')
            SIMHALT

FNAME:      DC.B    's02_temp.bin',0
PAYLOAD:    DC.B    '68K!'
            DS.W    0
            END     START
