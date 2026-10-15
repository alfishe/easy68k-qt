*-----------------------------------------------------------
* Title      : L1-09 Number I/O and Base Conversion
* Written by : EASy68K-Qt contributors
* Description: Read an integer with task 4 and print in base 10, 16, and 2
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVEQ   #4,D0           ; task 4: read number into D1.L
        TRAP    #15
        MOVE.L  D1,D3           ; save input in D3

        * Print in decimal (task 3)
        MOVEQ   #3,D0
        TRAP    #15

        MOVE.B  #' ',D1
        MOVEQ   #6,D0           ; task 6: display single char in D1.B
        TRAP    #15

        * Print in hexadecimal (task 15, base 16)
        MOVE.L  D3,D1
        MOVEQ   #16,D2
        MOVEQ   #15,D0
        TRAP    #15

        MOVE.B  #' ',D1
        MOVEQ   #6,D0
        TRAP    #15

        * Print in binary (task 15, base 2)
        MOVE.L  D3,D1
        MOVEQ   #2,D2
        MOVEQ   #15,D0
        TRAP    #15
        SIMHALT

        END     START
