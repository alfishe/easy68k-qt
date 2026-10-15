# Headless example runner library + CLI (Task E1).
add_library(easy68k-example-runner STATIC
  example_runner.cc
)
target_include_directories(easy68k-example-runner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(easy68k-example-runner PUBLIC easym68k-asm easym68k-sim)

add_executable(easy68k-run main.cc)
target_link_libraries(easy68k-run PRIVATE easy68k-example-runner)
