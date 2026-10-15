*-----------------------------------------------------------
* Title      : L3-07 Custom Stacks and Ring Buffer Queues
* Written by : EASy68K-Qt contributors
* Description: Contrast LIFO stack -(A2)/(A2)+ with FIFO ring buffer
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * 1. LIFO stack using A2 (grows downward from $2100)
        LEA     $2100,A2
        MOVE.L  #10,-(A2)       ; push 10
        MOVE.L  #20,-(A2)       ; push 20
        MOVE.L  #30,-(A2)       ; push 30
        MOVE.L  (A2)+,D0        ; pop -> D0 = 30 (LIFO)

        * 2. FIFO byte ring buffer at $2000 (capacity 8, mask = 7)
        LEA     $2000,A0
        MOVEQ   #0,D2           ; head (read index) = 0
        MOVEQ   #0,D3           ; tail (write index) = 0

        * Enqueue 10, 20, 30
        MOVE.B  #10,0(A0,D3.W)
        ADDQ.W  #1,D3
        ANDI.W  #7,D3
        MOVE.B  #20,0(A0,D3.W)
        ADDQ.W  #1,D3
        ANDI.W  #7,D3
        MOVE.B  #30,0(A0,D3.W)
        ADDQ.W  #1,D3
        ANDI.W  #7,D3

        * Dequeue first item -> D1 = 10 (FIFO)
        MOVEQ   #0,D1
        MOVE.B  0(A0,D2.W),D1
        ADDQ.W  #1,D2
        ANDI.W  #7,D2
        SIMHALT

        END     START
