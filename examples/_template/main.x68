*-----------------------------------------------------------
* Title      : <ID> <Title>
* Written by : EASy68K-Qt contributors
* Description: <one sentence: what this program demonstrates>
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:                          ; first instruction of program

* --- Put program code here ---------------------------------
        MOVEQ   #42,D0          ; replace with the example's code

        SIMHALT                 ; halt simulator

* --- Put variables and constants here ----------------------

        END     START           ; last line of source
