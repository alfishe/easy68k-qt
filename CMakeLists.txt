cmake_minimum_required(VERSION 3.16)
project(easym68k VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CompilerWarnings.cmake)

option(BUILD_TESTING "Build unit and golden tests" ON)
option(BUILD_GUI "Build Qt 6 GUI applications" ON)
option(ENABLE_SERIAL_PORT "Use Qt SerialPort for Trap #15 serial I/O if available" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Core libraries — zero Qt dependency
add_subdirectory(src/sim)
add_subdirectory(src/asm)

# Command-line tools (no Qt)
add_subdirectory(tools/easy68k-asm)
add_subdirectory(tools/easy68k-sim)

# Developer reference tools built from the original EASy68K sources (oracles
# for parity testing). Never part of the applications.
option(BUILD_REFERENCE_TOOLS "Build reference oracles from the original EASy68K sources" OFF)
if(BUILD_REFERENCE_TOOLS)
  add_subdirectory(tools/reference)
endif()

# Qt GUI applications
if(BUILD_GUI)
  find_package(Qt6 REQUIRED COMPONENTS
    Widgets Multimedia Network PrintSupport
  )
  # SerialPort is optional: without it, Trap #15 serial tasks (40-43) report
  # failure to the 68000 program instead of the build failing.
  # ENABLE_SERIAL_PORT=OFF disables it even when the module is installed.
  set(Qt6SerialPort_FOUND FALSE)
  if(ENABLE_SERIAL_PORT)
    find_package(Qt6 QUIET OPTIONAL_COMPONENTS SerialPort)
  endif()
  if(Qt6SerialPort_FOUND)
    message(STATUS "Qt6 SerialPort found: serial I/O enabled")
  elseif(ENABLE_SERIAL_PORT)
    message(STATUS "Qt6 SerialPort not found: serial I/O disabled")
  else()
    message(STATUS "ENABLE_SERIAL_PORT=OFF: serial I/O disabled")
  endif()
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
  add_subdirectory(gui)
endif()

# Tests
if(BUILD_TESTING)
  enable_testing()
  include(FetchContent)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  add_subdirectory(tests)
endif()
