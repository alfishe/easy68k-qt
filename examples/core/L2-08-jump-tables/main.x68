*-----------------------------------------------------------
* Title      : L2-08 Jump Tables and Switch Dispatch
* Written by : EASy68K-Qt contributors
* Description: Constant-time switch dispatch via a 4-entry jump table
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVEQ   #2,D0           ; case 2 = multiply
        MOVEQ   #7,D1           ; operand A = 7
        MOVEQ   #6,D2           ; operand B = 6

        CMP.W   #3,D0           ; bounds check: 0..3
        BHI.S   CASE_DEFAULT
        LSL.W   #2,D0           ; byte offset = case * 4
        LEA     JTABLE,A0
        MOVEA.L 0(A0,D0.W),A0
        JMP     (A0)

CASE_ADD:
        ADD.L   D2,D1
        BRA.S   DONE
CASE_SUB:
        SUB.L   D2,D1
        BRA.S   DONE
CASE_MUL:
        MULS.W  D2,D1           ; 7 * 6 = 42
        BRA.S   DONE
CASE_XOR:
        EOR.L   D2,D1
        BRA.S   DONE
CASE_DEFAULT:
        MOVEQ   #-1,D1
DONE:
        SIMHALT

JTABLE  DC.L    CASE_ADD,CASE_SUB,CASE_MUL,CASE_XOR

        END     START
