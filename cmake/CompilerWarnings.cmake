option(EASY68K_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(EASY68K_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
  endif()
  if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    # The Apple SDK unconditionally deprecates sprintf() in C++ mode and the
    # ported assembler core uses it pervasively (mirrors _CRT_SECURE_NO_WARNINGS
    # below). Warnings stay visible; they just stop failing the build.
    add_compile_options(-Wno-error=deprecated-declarations)
  endif()
  if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
  endif()
  if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # C runtime "secure" function deprecations do not apply to portable code.
  add_compile_options(/W4 /utf-8 /D_CRT_SECURE_NO_WARNINGS /DNOMINMAX /wd4100 /wd4244 /wd4267 /wd4458)
  if(EASY68K_WARNINGS_AS_ERRORS)
    add_compile_options(/WX)
  endif()
  # The assembler recurses one frame cycle per macro nesting level and the
  # original allows 256 levels (the diagnostics corpus listing encodes them);
  # Debug frames need ~17 KB each, far beyond the default 1 MB stack.
  add_link_options(/STACK:33554432)
endif()

# Source-based coverage (Task 12.2): tools/coverage.sh builds with this.
option(ENABLE_COVERAGE "Instrument for coverage (Clang: llvm-cov, GCC: gcov)" OFF)
if(ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(--coverage)
    add_link_options(--coverage)
  endif()
endif()
