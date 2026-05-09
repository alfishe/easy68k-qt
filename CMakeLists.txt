cmake_minimum_required(VERSION 3.16)
project(easym68k VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CompilerWarnings.cmake)

option(BUILD_TESTING "Build unit and golden tests" ON)
option(BUILD_GUI "Build Qt 6 GUI applications" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Core libraries — zero Qt dependency
add_subdirectory(src/sim)
add_subdirectory(src/asm)

# Qt GUI applications
if(BUILD_GUI)
  find_package(Qt6 REQUIRED COMPONENTS
    Widgets Multimedia SerialPort Network PrintSupport
  )
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
