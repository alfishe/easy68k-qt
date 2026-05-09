add_library(easym68k-asm STATIC
  assembler.cc
  lexer.cc
  parser.cc
  symbol_table.cc
  expression_evaluator.cc
  error_reporter.cc
  code_generator.cc
  directives.cc
  instruction_table.cc
  listing_generator.cc
  macro_processor.cc
)

target_include_directories(easym68k-asm PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_compile_features(easym68k-asm PUBLIC cxx_std_17)
target_link_libraries(easym68k-asm PRIVATE easym68k-sim)
