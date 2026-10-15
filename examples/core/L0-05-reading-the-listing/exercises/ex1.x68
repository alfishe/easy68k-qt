* L0-05 Exercise 1: Define MAGIC EQU $1234 and store at $2000
* SPDX-License-Identifier: CC0-1.0
MAGIC   EQU     $1234
        ORG     $1000
START:
        * TODO: load MAGIC into D0.W and store it at $2000
        SIMHALT
        END     START
