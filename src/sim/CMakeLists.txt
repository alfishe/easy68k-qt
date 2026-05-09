add_library(easym68k-sim STATIC
  memory.cc
  addressing_mode.cc
  flag_computation.cc
  simulator.cc
  decode.cc
  srecord_loader.cc
  trap15_dispatch.cc
  opcodes_move.cc
  opcodes_arithmetic.cc
  opcodes_logic.cc
  opcodes_branch.cc
  opcodes_misc.cc
  opcodes_shift.cc
)

target_include_directories(easym68k-sim PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_compile_features(easym68k-sim PUBLIC cxx_std_17)
