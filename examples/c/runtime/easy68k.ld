/*
 * easy68k.ld — Linker script for freestanding 68000 programs in EASy68K.
 * Places _start at $00001000 (standard EASy68K ORG $1000).
 * SPDX-License-Identifier: CC0-1.0
 */
OUTPUT_ARCH(m68k)
ENTRY(_start)

SECTIONS
{
  . = 0x00001000;

  .text : ALIGN(2)
  {
    KEEP(*(.text.startup))
    *(.text .text.*)
  }

  .rodata : ALIGN(2)
  {
    *(.rodata .rodata.*)
  }

  .init_array : ALIGN(4)
  {
    __init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP(*(.init_array .ctors))
    __init_array_end = .;
  }

  .data : ALIGN(2)
  {
    *(.data .data.*)
  }

  .bss : ALIGN(2)
  {
    __bss_start = .;
    *(.bss .bss.* COMMON)
    . = ALIGN(2);
    __bss_end = .;
  }

  /DISCARD/ :
  {
    *(.comment)
    *(.eh_frame*)
    *(.note*)
  }
}
