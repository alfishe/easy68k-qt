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
  sim/flag_computation_test.cc
  sim/exception_test.cc
  sim/trap15_mock_test.cc
  sim/golden_trace_test.cc
)
target_link_libraries(easym68k-sim-tests PRIVATE easym68k-sim GTest::gtest_main GTest::gmock)
add_test(NAME sim-tests COMMAND easym68k-sim-tests)

add_executable(easym68k-asm-tests
  asm/lexer_test.cc
  asm/parser_test.cc
  asm/symbol_table_test.cc
  asm/expression_test.cc
  asm/assembler_test.cc
  asm/code_generator_test.cc
  asm/instruction_table_test.cc
  asm/directives_test.cc
  asm/macro_test.cc
  asm/structured_test.cc
  asm/listing_test.cc
  asm/golden_assembly_test.cc
)
target_link_libraries(easym68k-asm-tests PRIVATE easym68k-asm GTest::gtest_main)
add_test(NAME asm-tests COMMAND easym68k-asm-tests)
