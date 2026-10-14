# Reference oracle: the ORIGINAL EASy68K simulator core (Sim68K: RUN.CPP,
# CODE1-9.CPP, UTILS.CPP, ...), built from the upstream Borland sources
# against a small VCL/GUI compatibility layer, as a 32-bit program (the
# sources assume 32-bit long and pointers). Developer tool only.
# -fno-toplevel-reorder keeps globals in declaration order: put() and
# value_of() tell registers from memory by comparing against &D[0]..&inst
# (var.h), which main.cc checks at start-up.

set(_sim68k "${EASY68K_ORIGINAL_SOURCE_DIR}/Sim68K")
if(NOT EXISTS "${_sim68k}/RUN.CPP")
  message(STATUS "Reference simulator: original sources not found at ${_sim68k}; skipped")
  return()
endif()
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS "-m32")
check_cxx_source_compiles("int main() { return 0; }" EASY68K_HAVE_M32)
unset(CMAKE_REQUIRED_FLAGS)
if(NOT EASY68K_HAVE_M32)
  message(STATUS "Reference simulator: no -m32 toolchain (g++-multilib); skipped")
  return()
endif()

set(_core_sources
  RUN.CPP CODE1.CPP CODE2.CPP CODE3.CPP CODE4.CPP CODE5.CPP CODE6.CPP CODE7.CPP
  CODE8.CPP CODE9.CPP UTILS.CPP STARTSIM.CPP SIMOPS1.CPP SIMOPS2.CPP STRUTILS.CPP SCAN.CPP
  BPoint.cpp BPointExpr.cpp
)
set(_core_headers def.h extern.h var.h PROTO.H OPCODES.H BPoint.h BPointExpr.h)

set(_stage "${CMAKE_CURRENT_BINARY_DIR}/original")
file(MAKE_DIRECTORY "${_stage}")
string(ASCII 26 _ctrl_z)
set(_staged_sources "")
foreach(_f IN LISTS _core_sources _core_headers)
  file(READ "${_sim68k}/${_f}" _content)
  string(REPLACE "${_ctrl_z}" "" _content "${_content}")
  string(REPLACE "\r" "" _content "${_content}")
  string(TOLOWER "${_f}" _lower)
  string(REGEX REPLACE "\\.cpp$" ".cc" _lower "${_lower}")
  # Borland-only constructs g++ rejects, rewritten with identical meaning.
  if(_lower STREQUAL "code7.cc")
    # Cast-as-lvalue: logical right shift of EV1.
    string(REPLACE "(uint)EV1 >>= 1;" "EV1 = (uint)EV1 >> 1;" _content "${_content}")
    # Address of a cast of an (unsigned) long: same object.
    string(REPLACE "&(static_cast<long>(bit_no))" "(long*)&bit_no" _content "${_content}")
  endif()
  file(WRITE "${_stage}/${_lower}" "${_content}")
  if(NOT _lower MATCHES "\\.cc$" AND NOT _lower STREQUAL _f)
    # Sources include headers with mixed case ("BPoint.h", "PROTO.H").
    file(WRITE "${_stage}/${_f}" "${_content}")
  endif()
  if(_lower MATCHES "\\.cc$")
    list(APPEND _staged_sources "${_stage}/${_lower}")
  endif()
endforeach()

# The sources include both "net.h" and "Net.h"; only one may live in the
# repository (case-insensitive checkouts), so the other is generated here.
file(WRITE "${_stage}/Net.h" "#include \"sim_gui_stubs.h\"\n")

add_executable(easy68k-ref-sim main.cc stubs.cc ${_staged_sources})
target_include_directories(easy68k-ref-sim PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/compat" "${_stage}")
target_compile_options(easy68k-ref-sim PRIVATE
  -m32 -w -fpermissive -fno-operator-names -fno-toplevel-reorder
  -include "${CMAKE_CURRENT_SOURCE_DIR}/compat/sim_prelude.h")
target_link_options(easy68k-ref-sim PRIVATE -m32)
