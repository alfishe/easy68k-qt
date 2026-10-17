cmake_minimum_required(VERSION 3.16)
project(easym68k VERSION 1.0.0 LANGUAGES CXX)

# After project(): APPLE is defined, so the module can act on it.
include(cmake/MacOSDeploymentTarget.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CompilerWarnings.cmake)
include(cmake/GuiAppBundle.cmake)

option(BUILD_TESTING "Build unit and golden tests" ON)
option(BUILD_GUI "Build Qt 6 GUI applications" ON)
option(BUILD_EXAMPLES_C "Build Track C/C++ examples when m68k GCC cross-compiler is present" ON)
option(ENABLE_SERIAL_PORT "Use Qt SerialPort for Trap #15 serial I/O if available" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Core libraries — zero Qt dependency
add_subdirectory(src/sim)
add_subdirectory(src/asm)

# Command-line tools (no Qt)
add_subdirectory(tools/easy68k-asm)
add_subdirectory(tools/easy68k-sim)
add_subdirectory(tools/easy68k-run)

if(BUILD_EXAMPLES_C)
  add_subdirectory(examples/c)
endif()

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

# Suite packaging targets (release builds; no-op without BUILD_GUI)
include(cmake/SuitePackaging.cmake)

# Tests
if(BUILD_TESTING)
  enable_testing()
  include(FetchContent)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  add_subdirectory(tests)
endif()

# Installation and CPack packaging. Runtime artifacts belong to the Suite
# component (what the DEB ships); headers and static libraries to Devel.
include(GNUInstallDirs)

install(TARGETS easym68k-sim easym68k-asm
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT Devel
)
install(DIRECTORY include/easym68k
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT Devel
)
install(TARGETS easy68k-asm easy68k-sim easy68k-run
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT Suite
)
if(BUILD_GUI)
  install(TARGETS easy68k-simulator easy68k-editor easybin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
    COMPONENT Suite
  )
endif()
install(DIRECTORY data/help
  DESTINATION ${CMAKE_INSTALL_DATADIR}/easy68k
  COMPONENT Suite
)
install(FILES data/template.NEW
  DESTINATION ${CMAKE_INSTALL_DATADIR}/easy68k
  COMPONENT Suite
)
install(DIRECTORY examples
  DESTINATION ${CMAKE_INSTALL_DATADIR}/easy68k
  COMPONENT Suite
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND BUILD_GUI)
  install(DIRECTORY install/linux/icons/hicolor
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons
    COMPONENT Suite
  )
  install(FILES
    install/linux/easy68k-simulator.desktop
    install/linux/easy68k-editor.desktop
    install/linux/easybin.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    COMPONENT Suite
  )
endif()

set(CPACK_PACKAGE_NAME "easy68k-qt")
set(CPACK_PACKAGE_VENDOR "EASy68K Contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform Motorola 68000 assembler, simulator, and curriculum")
if(EASY68K_PACKAGE_VERSION)
  set(CPACK_PACKAGE_VERSION "${EASY68K_PACKAGE_VERSION}")
else()
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
endif()
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND BUILD_GUI)
  # Distro-Qt deb: Depends computed by dpkg-shlibdeps against the build Qt.
  set(CPACK_GENERATOR "TGZ;DEB")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
  set(CPACK_PACKAGE_FILE_NAME "easy68k-qt-Linux-${CMAKE_SYSTEM_PROCESSOR}")
  set(CPACK_COMPONENTS_ALL Suite)
  set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "EASy68K Contributors")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_SECTION "education")
else()
  set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
