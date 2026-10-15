* Trap #15 file tasks 50-57 and 59 (Task 10.4), compared with the original
* Sim68K core (each simulator runs in an empty directory; files left behind
* are compared too). Cases where the port deliberately differs (EOF read
* size, zero-length write, file-exists test on a missing file) are covered
* by unit tests instead. CC0-1.0.
START   ORG     $1000
        LEA     NAME1,A1
        MOVEQ   #52,D0          ; new file
        TRAP    #15
        MOVE.L  D1,D7           ; FID
        LEA     DATA,A1
        MOVE.L  #16,D2
        MOVEQ   #54,D0          ; write 16 bytes
        TRAP    #15
        MOVE.L  #4,D2
        MOVEQ   #55,D0          ; position 4
        TRAP    #15
        LEA     BUF,A1
        MOVE.L  #8,D2
        MOVEQ   #53,D0          ; read 8 bytes
        TRAP    #15
        MOVE.L  #100,D2
        MOVEQ   #53,D0          ; read the remaining 4 (partial read)
        TRAP    #15
        MOVEQ   #56,D0          ; close
        TRAP    #15
        MOVEQ   #56,D0          ; close again: error
        TRAP    #15
        LEA     NAME1,A1
        MOVEQ   #51,D0          ; open existing
        TRAP    #15
        MOVE.L  D1,D6
        LEA     BUF2,A1
        MOVE.L  #16,D2
        MOVEQ   #53,D0          ; read all 16
        TRAP    #15
        LEA     NAME1,A1
        MOVEQ   #59,D0          ; file exists (and writable)
        MOVEQ   #0,D1
        TRAP    #15
        LEA     NONAME,A1
        MOVEQ   #51,D0          ; open missing file: error, FID -1
        TRAP    #15
        MOVE.L  #99,D1
        MOVEQ   #53,D0          ; read invalid FID: error
        TRAP    #15
* fill the file table (8 entries)
        MOVEQ   #6,D5
OPENLP  LEA     NAME1,A1
        MOVEQ   #51,D0
        TRAP    #15
        DBRA    D5,OPENLP
        LEA     NAME1,A1
        MOVEQ   #51,D0          ; ninth: error
        TRAP    #15
        MOVEQ   #50,D0          ; close all
        TRAP    #15
        LEA     NAME1,A1
        MOVEQ   #57,D0          ; delete
        TRAP    #15
        MOVEQ   #57,D0          ; delete again: error
        TRAP    #15
* leave one file behind
        LEA     NAME2,A1
        MOVEQ   #52,D0
        TRAP    #15
        LEA     DATA,A1
        MOVE.L  #10,D2
        MOVEQ   #54,D0
        TRAP    #15
        MOVEQ   #56,D0
        TRAP    #15
* buffer beyond the end of memory: error without access
        MOVE.L  D1,D1
        LEA     $FFFFF0,A1
        MOVE.L  #$20,D2
        MOVEQ   #54,D0
        TRAP    #15
        MOVEQ   #9,D0
        TRAP    #15
NAME1   DC.B    'test.dat',0
NAME2   DC.B    'keep.txt',0
NONAME  DC.B    'missing.dat',0
DATA    DC.B    '0123456789ABCDEF'
        DS.W    0
BUF     DS.B    16
BUF2    DS.B    16
        END     START
