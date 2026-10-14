add_executable(easym68k-sim-tests
  sim/memory_test.cc
  sim/effective_addr_test.cc
  sim/addressing_mode_test.cc
  sim/simulator_test.cc
  sim/decode_test.cc
  sim/srecord_loader_test.cc
  sim/opcodes_move_test.cc
  sim/opcodes_arithmetic_test.cc
  sim/opcodes_logic_test.cc
  sim/opcodes_branch_test.cc
  sim/opcodes_misc_test.cc
  sim/opcodes_shift_test.cc
  sim/bitfield_test.cc
  sim/break_condition_test.cc
  sim/flag_computation_test.cc
  sim/exception_test.cc
  sim/trap15_mock_test.cc
  sim/golden_trace_test.cc
  sim/interrupt_test.cc
  sim/cycle_test.cc
  sim/execution_logger_test.cc
  sim/observer_test.cc
  sim/address_bus_error_test.cc
)
target_link_libraries(easym68k-sim-tests PRIVATE easym68k-sim GTest::gtest_main GTest::gmock)
add_test(NAME sim-tests COMMAND easym68k-sim-tests)

add_executable(easym68k-asm-tests
  asm/assembler_test.cc
  asm/movem_test.cc
  asm/directives_test.cc
  asm/conditional_assembly_test.cc
  asm/macro_test.cc
  asm/structured_test.cc
  asm/error_reporter_test.cc
  asm/golden_assembly_test.cc
  asm/instruction_matrix_test.cc
  asm/corpus_test.cc
)
target_link_libraries(easym68k-asm-tests PRIVATE easym68k-asm GTest::gtest_main)
add_test(NAME asm-tests COMMAND easym68k-asm-tests
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Cross-library integration tests (assembler + simulator), Task 9.6.
add_executable(easym68k-integration-tests
  integration/core_integration_test.cc
)
target_link_libraries(easym68k-integration-tests PRIVATE
  easym68k-asm easym68k-sim GTest::gtest_main
)
add_test(NAME integration-tests COMMAND easym68k-integration-tests)
