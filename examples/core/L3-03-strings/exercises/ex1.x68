* L3-03 Exercise 1: Count characters in STR ('EASy68K',0) -> D0 = 7
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     STR,A0
        MOVEQ   #0,D0
        * TODO: count non-null bytes at (A0)+ into D0
        SIMHALT

STR     DC.B    'EASy68K',0
        END     START
