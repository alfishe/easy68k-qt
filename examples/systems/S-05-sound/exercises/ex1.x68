*-----------------------------------------------------------
* Exercise S-05-EX1: Load slot 2 and stop-all via task 76
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Load WAV_NAME into slot 2 (task 71), stop all via task 76
            * (D2=3), and set D0 = 76
            SIMHALT
WAV_NAME:   DC.B    'tone.wav',0
            DS.W    0
            END     START
