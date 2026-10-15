* Assembler corpus: operand syntax variety (index registers with sizes,
* PC-relative index, SP/A7, negative and forced-size displacements,
* absolute .W/.L, immediates of every size, register lists) and the
* related diagnostics, compared with the original assembler. CC0-1.0.
        ORG     $1000
START   MOVE.W  (A0,D1.W),D0
        MOVE.W  (A0,D1.L),D0
        MOVE.W  (A0,A1),D0
        MOVE.W  (A0,A1.W),D0
        MOVE.W  (A0,A1.L),D0
        MOVE.W  (SP,D2.W),D0
        MOVE.W  (A6,SP.L),D0
        MOVE.W  2(A0,D1.W),D0
        MOVE.W  -2(A0,D1.L),D0
        MOVE.W  $7F(A0,A2.W),D0
        MOVE.W  -$80(A0,A2.L),D0
        MOVE.W  (2,A0),D0
        MOVE.W  (2,A0,D3.W),D0
        MOVE.W  (-4,SP,D3.L),D0
        MOVE.W  4(SP),D0
        MOVE.W  -4(A7),D0
        MOVE.W  $7FFF(A3),D0
        MOVE.W  -$8000(A3),D0
        MOVE.W  START(PC),D0
        MOVE.W  START(PC,D0.W),D0
        MOVE.W  START(PC,A0.L),D0
        MOVE.W  (START,PC),D0
        MOVE.W  (START,PC,D1.W),D0
        MOVE.W  *(PC),D0
        MOVE.W  $1234.W,D0
        MOVE.W  $1234.L,D0
        MOVE.W  $FFFF8000.W,D0
        MOVE.W  $00012345,D0
        MOVE.B  #-1,D0
        MOVE.B  #$FF,D0
        MOVE.W  #-32768,D0
        MOVE.W  #$FFFF,D0
        MOVE.L  #-1,D0
        MOVE.L  #START,A0
        MOVE.L  #'AB',D0
        MOVE.L  #'ABCD',D0
        ADD.L   #$12345678,(A1)+
        SUB.B   #1,-(A2)
        CMP.W   #2,4(A3)
        MOVEM.L D0-D7/A0-A6,-(SP)
        MOVEM.L (SP)+,D0-D7/A0-A6
        MOVEM.W D0/D2/D4/A1,$2000
        MOVEM.L $2000.W,D1-D3
        MOVEM.W 2(A0,D1.W),A1-A2
        MOVEM.L A0,-(A7)
        LEA     (A0,D1.W),A2
        LEA     START(PC,D2.L),A3
        PEA     (4,SP)
        JMP     (A0,D0.W)
        JSR     START(PC,D1.W)
* diagnostics
        MOVE.W  (A0,D1.X),D0            invalid index size
        MOVE.W  (A0,D9),D0              not a register
        MOVE.W  $80(A0,D1.W),D0         displacement out of range
        MOVE.W  $8000(A3),D0            displacement out of range
        MOVE.W  (A0,D1.W,D0)            syntax
        MOVE.W  (A0,,D1),D0             syntax
        MOVE.W  #$10000,D0              exceeds 16 bits
        MOVE.B  #$100,D0                exceeds 8 bits
        MOVEM.L D0-,(A0)                syntax
        MOVEM.L D0/D9,(A0)              syntax
        SIMHALT
        END     START
