# Headless runner with register tracing; the library is shared with the
# golden simulation trace test.
add_library(easy68k-sim-trace STATIC trace_runner.cc)
target_include_directories(easy68k-sim-trace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(easy68k-sim-trace PUBLIC easym68k-sim)

add_executable(easy68k-sim main.cc)
target_link_libraries(easy68k-sim PRIVATE easy68k-sim-trace)
