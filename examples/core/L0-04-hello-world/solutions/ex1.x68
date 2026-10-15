* L0-04 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     MSG,A1
        MOVEQ   #13,D0
        TRAP    #15
        SIMHALT

MSG     DC.B    'EASy68K',0
        END     START
