/*
 * crt0.S — Freestanding 68000 startup code for EASy68K ($1000 entry point).
 * Initializes stack, clears .bss, runs C++ global constructors (__init_array),
 * invokes main(), and halts via SIMHALT while preserving main()'s return value
 * in D0.
 *
 * SPDX-License-Identifier: CC0-1.0
 */
    .section .text.startup, "ax", @progbits
    .globl _start
    .type _start, @function
_start:
    /* Set user/supervisor stack pointer to $00FF0000 */
    lea     0x00FF0000, %sp

    /* Zero .bss [__bss_start .. __bss_end) */
    lea     __bss_start, %a0
    lea     __bss_end, %a1
    bra.s   .Lbss_check
.Lbss_loop:
    clr.b   (%a0)+
.Lbss_check:
    cmpa.l  %a1, %a0
    blo.s   .Lbss_loop

    /* Call C++ static constructors in [__init_array_start .. __init_array_end) */
    lea     __init_array_start, %a2
    lea     __init_array_end, %a3
    bra.s   .Linit_check
.Linit_loop:
    movea.l (%a2)+, %a0
    jsr     (%a0)
.Linit_check:
    cmpa.l  %a3, %a2
    blo.s   .Linit_loop

    /* Call main(); return value is in %d0 */
    jsr     main

    /* SIMHALT ($FFFF) halts Sim68K without clobbering %d0 */
    .word   0xFFFF
.Lhang:
    bra.s   .Lhang
