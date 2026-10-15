*-----------------------------------------------------------
* Exercise S-16-EX1: Measure a single NOP with tasks 30 and 31
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Clear cycles (task 30), execute one NOP, read cycles (task 31)
            * and copy D1 into D0
            SIMHALT
            END     START
