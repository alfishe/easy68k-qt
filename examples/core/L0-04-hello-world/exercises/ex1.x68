* L0-04 Exercise 1: Print "EASy68K" with CRLF
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        * TODO: load MSG into A1 and invoke Trap #15 task 13
        SIMHALT

MSG     DC.B    'EASy68K',0
        END     START
