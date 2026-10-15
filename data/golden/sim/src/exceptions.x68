* Exception processing (Task 11.1), compared with the original Sim68K core
* (run with exceptions enabled, see exceptions.flags). Each case points
* NEXT at its continuation; the common handler copies the stack frame
* (14 bytes) to a log, resets the stack and resumes at NEXT. The TRAP case
* returns with RTE instead. CC0-1.0.
NEXT    EQU     $7F00
LOGPTR  EQU     $7F04
SSTK    EQU     $8000
LOG     EQU     $6000
        ORG     0
        DC.L    SSTK,START
        DCB.L   30,HANDLER      ; vectors 2-31
        DC.L    HANDLER,HANDLER,HANDLER,TRAP3   ; TRAP #0-#3
        DCB.L   28,HANDLER      ; vectors 36-63

        ORG     $1000
HANDLER MOVEA.L LOGPTR,A6       ; copy the frame
        MOVE.L  (SP),(A6)+
        MOVE.L  4(SP),(A6)+
        MOVE.L  8(SP),(A6)+
        MOVE.W  12(SP),(A6)+
        MOVE.L  A6,LOGPTR
        LEA     SSTK,SP
        MOVE    #$2700,SR
        MOVEA.L NEXT,A6
        JMP     (A6)

TRAP3   ADDQ.L  #1,D7           ; count, then return with RTE
        RTE

START   MOVE.L  #LOG,LOGPTR
* address error: word read at an odd address
        MOVE.L  #C1,NEXT
        LEA     $3001,A0
        MOVE.W  (A0),D0
C1      MOVE.L  #C2,NEXT
* address error on a read after an extension word (stacked PC is the PC at
* the read). (Write faults: the original reads a MOVE destination first and
* stacks a read frame, an accepted deviation; see exception_test.cc.)
        MOVE.W  $3003,D0
C2      MOVE.L  #C3,NEXT
* (privilege violation: the port stacks the instruction address, the
* original the PC after the first word; see exception_test.cc.)
C3      MOVE.L  #C4,NEXT
* illegal instruction
        ILLEGAL
C4      MOVE.L  #C5,NEXT
* divide by zero
        MOVEQ   #0,D1
        DIVU    D1,D0
C5      MOVE.L  #C6,NEXT
* (CHK: Z/V/C are undefined and the original inverts them, an accepted
* deviation; CHK exceptions are covered by tests/sim/exception_test.cc.)
C6      MOVE.L  #C7,NEXT
C7      MOVE.L  #C8,NEXT
* TRAPV with V set
        MOVE    #$2702,SR
        TRAPV
C8      MOVE.L  #C9,NEXT
* trace: T bit set, the next instruction is followed by a trace exception
        MOVE    #$A700,SR
        NOP
C9      MOVE.L  #C10,NEXT
* Line 1010 and Line 1111
        DC.W    $A123
C10     MOVE.L  #C11,NEXT
        DC.W    $F123
C11     MOVE.L  #C12,NEXT
* TRAP #3: handler returns with RTE (restores SR and PC)
        MOVE    #$2715,SR
        TRAP    #3
        MOVE    SR,D6           ; SR restored by RTE
        TRAP    #3
C12     MOVEQ   #9,D0
        TRAP    #15
        END     START
