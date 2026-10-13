# Reference oracle: the ORIGINAL EASy68K assembler (Edit68K core), built from
# the upstream Borland sources against a small VCL compatibility layer.
# Developer tool only; never installed or linked into the applications.
#
# Configure with -DEASY68K_ORIGINAL_SOURCE_DIR=<path to EASy68K checkout>
# (default: ../Easy68k next to this repository).

set(_edit68k "${EASY68K_ORIGINAL_SOURCE_DIR}/Edit68K")
if(NOT EXISTS "${_edit68k}/ASSEMBLE.CPP")
  message(STATUS "Reference assembler: original sources not found at ${_edit68k}; skipped")
  return()
endif()

set(_core_sources
  ASSEMBLE.CPP BUILD.CPP CODEGEN.CPP DIRECTIV.CPP ERROR.CPP EVAL.CPP GLOBALS.CPP
  INSTLOOK.CPP INSTTABL.CPP LISTING.CPP MACRO.CPP MOVEM.CPP OPPARSE.CPP
  OBJECT.CPP STRUCTURED.CPP SYMBOL.CPP
)
set(_core_headers asm.h proto.h)

# Stage copies so that quoted includes of GUI form headers (mainS.h, textS.h,
# ...) resolve to compat/ instead of the originals next to the sources, and
# strip the DOS end-of-file marker (Ctrl-Z) that GCC rejects.
set(_stage "${CMAKE_CURRENT_BINARY_DIR}/original")
file(MAKE_DIRECTORY "${_stage}")
string(ASCII 26 _ctrl_z)
set(_staged_sources "")
foreach(_f IN LISTS _core_sources _core_headers)
  file(READ "${_edit68k}/${_f}" _content)
  string(REPLACE "${_ctrl_z}" "" _content "${_content}")
  string(REPLACE "\r" "" _content "${_content}")
  # The only source fix-up: implicit-int declaration rejected by modern C++.
  string(REPLACE "extern lineNumL68;" "extern int lineNumL68;" _content "${_content}")
  string(TOLOWER "${_f}" _lower)
  string(REGEX REPLACE "\\.cpp$" ".cc" _lower "${_lower}")
  file(WRITE "${_stage}/${_lower}" "${_content}")
  if(_lower MATCHES "\\.cc$")
    list(APPEND _staged_sources "${_stage}/${_lower}")
  endif()
endforeach()

add_executable(easy68k-ref-asm main.cc ${_staged_sources})
target_include_directories(easy68k-ref-asm PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/compat" "${_stage}")
target_compile_options(easy68k-ref-asm PRIVATE
  -w -fpermissive -fno-operator-names
  -include "${CMAKE_CURRENT_SOURCE_DIR}/compat/vcl.h")
