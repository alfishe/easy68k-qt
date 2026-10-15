* L3-07 Exercise 1: Push 100 and 200 on A2 stack and pop into D0, D1
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     $2100,A2
        * TODO: push 100, push 200, pop D0, pop D1
        SIMHALT
        END     START
