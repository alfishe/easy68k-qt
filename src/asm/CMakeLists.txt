add_library(easym68k-asm STATIC
  assembler.cc
  error_reporter.cc
  instruction_set.cc
  original_status.cc
  core/assemble.cc
  core/codegen.cc
  core/directives.cc
  core/error.cc
  core/eval.cc
  core/instlook.cc
  core/listing.cc
  core/macro.cc
  core/movem.cc
  core/object.cc
  core/opparse.cc
  core/structured.cc
  core/support.cc
  core/symbol.cc
)

target_include_directories(easym68k-asm PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_compile_features(easym68k-asm PUBLIC cxx_std_17)
target_link_libraries(easym68k-asm PRIVATE easym68k-sim)

# The core/ sources are ported line by line from the original EASy68K (see
# core/asm_core.h) and keep its C idioms (dangling else, unused locals,
# "return NULL" from int functions, printf formats for 32-bit longs), so
# those warnings are not errors there.
if(MSVC)
  # Same reason as below: warnings of the original C idioms are not errors.
  set_source_files_properties(
    core/assemble.cc core/codegen.cc core/directives.cc core/error.cc core/eval.cc
    core/instlook.cc core/listing.cc core/macro.cc core/movem.cc core/object.cc
    core/opparse.cc core/structured.cc core/support.cc core/symbol.cc
    PROPERTIES COMPILE_OPTIONS "/W1;/WX-")
else()
  set_source_files_properties(
    core/assemble.cc core/codegen.cc core/directives.cc core/error.cc core/eval.cc
    core/instlook.cc core/listing.cc core/macro.cc core/movem.cc core/object.cc
    core/opparse.cc core/structured.cc core/support.cc core/symbol.cc
    PROPERTIES COMPILE_OPTIONS
    "-Wno-dangling-else;-Wno-unused-variable;-Wno-unused-but-set-variable;-Wno-unused-parameter;-Wno-misleading-indentation;-Wno-format;-Wno-conversion-null;-Wno-parentheses;-Wno-sign-compare;-Wno-char-subscripts;-Wno-implicit-fallthrough;-Wno-write-strings;-Wno-unused-value;-Wno-format-security;-Wno-pedantic;-Wno-missing-braces")
endif()
