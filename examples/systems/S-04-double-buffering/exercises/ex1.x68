*-----------------------------------------------------------
* Exercise S-04-EX1: 5-frame pixel animation with Repaint
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Enable double buffering, draw pixel at X=10,20,30,40,50 (Y=25)
            * with task 94 each frame, and leave final X (50) in D0
            SIMHALT
            END     START
