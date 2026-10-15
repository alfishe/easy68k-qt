* L2-03 Exercise 1: Add 3 to D0 in a REPEAT..UNTIL D0 <GE> #10 loop
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #0,D0
        * TODO: REPEAT adding 3 to D0 UNTIL.L D0 <GE> #10 DO.S
        SIMHALT
        END     START
