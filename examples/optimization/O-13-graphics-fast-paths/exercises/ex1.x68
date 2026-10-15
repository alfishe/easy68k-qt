*-----------------------------------------------------------
* Exercise O-13-EX1: Fill (10,10)..(25,13) + Repaint in <= 120 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive pixel loop (exceeds 120 cycles! Replace with task 87 + task 94)
            MOVEQ   #10,D2
.YLP:       MOVEQ   #10,D1
.XLP:       MOVEQ   #82,D0
            TRAP    #15
            ADDQ.W  #1,D1
            CMPI.W  #26,D1
            BLT.S   .XLP
            ADDQ.W  #1,D2
            CMPI.W  #14,D2
            BLT.S   .YLP
            MOVEQ   #94,D0
            TRAP    #15
            SIMHALT
            END     START
