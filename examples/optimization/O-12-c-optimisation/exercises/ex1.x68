*-----------------------------------------------------------
* Exercise O-12-EX1: Scale {10,20,30,40} by 3 and sum (300) in <= 220 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Sum 3 * ARR16[i] for i=0..3 into D0 using (A0)+ and DBRA
            SIMHALT
ARR16:      DC.W    10,20,30,40
            END     START
