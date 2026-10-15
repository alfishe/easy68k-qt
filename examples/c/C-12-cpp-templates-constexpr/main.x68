*-----------------------------------------------------------
* Title      : C-12 Templates and constexpr on the 68000
* Description: Compile-time folding + template shift specialization
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            * runtime_x = 5; scale_pow2<3>(5) -> ASL.L #3,D0 (40)
            MOVEQ   #5,D0
            ASL.L   #3,D0               ; 40
            * kFact6 = factorial_ct(6) = 720 folded at compile time!
            ADD.L   #720,D0             ; D0 = 760
            SIMHALT
            END     _start
