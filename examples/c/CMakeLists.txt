# Track C/C++ Cross-Compilation Support (BUILD_EXAMPLES_C)
# SPDX-License-Identifier: CC0-1.0

find_program(M68K_GCC NAMES m68k-linux-gnu-gcc m68k-elf-gcc)
find_program(M68K_GXX NAMES m68k-linux-gnu-g++ m68k-elf-g++)
find_program(M68K_OBJCOPY NAMES m68k-linux-gnu-objcopy m68k-elf-objcopy)

if(M68K_GCC AND M68K_OBJCOPY)
  message(STATUS "Found m68k cross-compiler: ${M68K_GCC}")
  add_custom_target(easy68k-examples-c
    COMMENT "Track C/C++ cross-compiler available (${M68K_GCC})"
  )
else()
  message(STATUS "m68k GCC cross-compiler not found: using pre-assembled Track C/C++ S-records")
endif()
