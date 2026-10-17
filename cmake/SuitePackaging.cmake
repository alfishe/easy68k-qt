# Suite packaging targets (release builds): one distributable package per
# platform containing the three GUI apps, the three CLI tools, and the runtime
# data (help, examples, template). Artifacts land in ${CMAKE_BINARY_DIR}/packages.
#
# Targets:
#   package_suite_macos    easy68k-qt-Suite.dmg         (private Qt per bundle)
#   package_suite_windows  easy68k-qt-Suite-Windows.zip (private Qt, flat root)
#   package_suite_linux    packages/easy68k-qt-Suite/   (bin/+share staging tree
#                          consumed by tools/package-appimage.sh and cpack DEB)
#
# The deployqt tools are found via Qt6_DIR (any Qt installation method works).

if(NOT BUILD_GUI)
  return()
endif()

set(EASY68K_PACKAGE_VERSION "" CACHE STRING "Package version (YYYYMMDD.HHMMSS)")
if(NOT EASY68K_PACKAGE_VERSION)
  string(TIMESTAMP _easy68k_timestamp_version "%Y%m%d.%H%M%S" UTC)
  set(EASY68K_PACKAGE_VERSION "${_easy68k_timestamp_version}")
endif()

set(EASY68K_SUITE_STAGING "${CMAKE_BINARY_DIR}/packages/easy68k-qt-Suite")
set(EASY68K_GUI_APPS easy68k-simulator easy68k-editor easybin)
set(EASY68K_CLI_TOOLS easy68k-asm easy68k-sim easy68k-run)
set(EASY68K_SUITE_DEPS ${EASY68K_GUI_APPS} ${EASY68K_CLI_TOOLS})

get_filename_component(_qt_bin_hint "${Qt6_DIR}" DIRECTORY)  # .../lib/cmake
get_filename_component(_qt_bin_hint "${_qt_bin_hint}" DIRECTORY)  # .../lib
find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt windeployqt6
  HINTS "${_qt_bin_hint}/../bin" "${_qt_bin_hint}/bin")
find_program(MACDEPLOYQT_EXECUTABLE NAMES macdeployqt macdeployqt6
  HINTS "${_qt_bin_hint}/../bin" "${_qt_bin_hint}/bin")

# ---------------------------------------------------------------------------
# macOS: one DMG with three .app bundles, each carrying its own Qt frameworks
# (macdeployqt per bundle), the shared data duplicated into Contents/Resources
# (found via the ../Resources/data/... probe candidates), plus the CLI tools.
# ---------------------------------------------------------------------------
if(APPLE)
  set(_macos_cmds
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/packages"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${EASY68K_SUITE_STAGING}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EASY68K_SUITE_STAGING}"
  )
  foreach(_app IN LISTS EASY68K_GUI_APPS)
    # cp -a (not cmake -E copy_directory) to PRESERVE the framework symlinks.
    list(APPEND _macos_cmds
      COMMAND cp -a "$<TARGET_BUNDLE_DIR:${_app}>" "${EASY68K_SUITE_STAGING}/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/data/help"
        "${EASY68K_SUITE_STAGING}/${_app}.app/Contents/Resources/data/help"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/examples"
        "${EASY68K_SUITE_STAGING}/${_app}.app/Contents/Resources/data/examples"
      COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/data/template.NEW"
        "${EASY68K_SUITE_STAGING}/${_app}.app/Contents/Resources/data/template.NEW"
      COMMAND "${MACDEPLOYQT_EXECUTABLE}"
        "${EASY68K_SUITE_STAGING}/${_app}.app" -always-overwrite
      COMMAND codesign --force --deep --sign -
        "${EASY68K_SUITE_STAGING}/${_app}.app"
    )
  endforeach()
  foreach(_tool IN LISTS EASY68K_CLI_TOOLS)
    list(APPEND _macos_cmds
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${_tool}>" "${EASY68K_SUITE_STAGING}/"
      COMMAND codesign --force --sign - "${EASY68K_SUITE_STAGING}/${_tool}"
    )
  endforeach()
  list(APPEND _macos_cmds
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/LICENSE" "${EASY68K_SUITE_STAGING}/"
    COMMAND hdiutil create -volname "EASy68K Suite"
      -srcfolder "${EASY68K_SUITE_STAGING}" -ov -format UDZO
      "${CMAKE_BINARY_DIR}/packages/easy68k-qt-Suite.dmg"
  )
  add_custom_target(package_suite_macos
    COMMAND ${_macos_cmds}
    DEPENDS ${EASY68K_SUITE_DEPS}
    COMMENT "Packaging easy68k-qt-Suite.dmg")
endif()

# ---------------------------------------------------------------------------
# Windows: portable zip with a flat root. All executables sit next to each
# other (the spawn probes and data/data-help probes expect that), windeployqt
# installs one shared Qt DLL set into the root, and the MSVC CRT DLLs are
# copied alongside.
# ---------------------------------------------------------------------------
if(WIN32)
  set(_win_cmds
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/packages"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${EASY68K_SUITE_STAGING}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EASY68K_SUITE_STAGING}"
  )
  foreach(_target IN LISTS EASY68K_SUITE_DEPS)
    list(APPEND _win_cmds
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${_target}>" "${EASY68K_SUITE_STAGING}/")
  endforeach()
  set(_windeployqt_flags --no-translations --no-system-d3d-compiler --no-opengl-sw)
  if(MSVC)
    # Copy the CRT DLLs ourselves so the package runs without the redist
    # installed, and keep windeployqt from duplicating them.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
    foreach(_lib IN LISTS CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
      list(APPEND _win_cmds
        COMMAND ${CMAKE_COMMAND} -E copy "${_lib}" "${EASY68K_SUITE_STAGING}/")
    endforeach()
    list(APPEND _windeployqt_flags --no-compiler-runtime)
  endif()
  foreach(_app IN LISTS EASY68K_GUI_APPS)
    list(APPEND _win_cmds
      COMMAND "${WINDEPLOYQT_EXECUTABLE}"
        "${EASY68K_SUITE_STAGING}/${_app}.exe" ${_windeployqt_flags})
  endforeach()
  list(APPEND _win_cmds
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/data/help" "${EASY68K_SUITE_STAGING}/data/help"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/examples" "${EASY68K_SUITE_STAGING}/examples"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/data/template.NEW" "${EASY68K_SUITE_STAGING}/data/template.NEW"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/LICENSE" "${EASY68K_SUITE_STAGING}/"
    COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/packages"
      ${CMAKE_COMMAND} -E tar "cf" "easy68k-qt-Suite-Windows.zip"
      "--format=zip" "--" "easy68k-qt-Suite"
  )
  add_custom_target(package_suite_windows
    COMMAND ${_win_cmds}
    DEPENDS ${EASY68K_SUITE_DEPS}
    COMMENT "Packaging easy68k-qt-Suite-Windows.zip")
endif()

# ---------------------------------------------------------------------------
# Linux: stage the classic bin/ + share/ tree consumed by
# tools/package-appimage.sh (AppImage) and by cpack -G DEB (via the install
# rules' Suite component). Headers and static libraries stay out.
# ---------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
  set(_linux_cmds
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/packages"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${EASY68K_SUITE_STAGING}"
  )
  foreach(_target IN LISTS EASY68K_SUITE_DEPS)
    list(APPEND _linux_cmds
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${_target}>" "${EASY68K_SUITE_STAGING}/bin/")
  endforeach()
  list(APPEND _linux_cmds
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/data/help" "${EASY68K_SUITE_STAGING}/share/easy68k/help"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/data/template.NEW" "${EASY68K_SUITE_STAGING}/share/easy68k/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/examples" "${EASY68K_SUITE_STAGING}/share/easy68k/examples"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/install/linux/icons/hicolor"
      "${EASY68K_SUITE_STAGING}/share/icons/hicolor"
  )
  foreach(_app IN LISTS EASY68K_GUI_APPS)
    list(APPEND _linux_cmds
      COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/install/linux/${_app}.desktop"
        "${EASY68K_SUITE_STAGING}/share/applications/")
  endforeach()
  list(APPEND _linux_cmds
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/LICENSE" "${EASY68K_SUITE_STAGING}/"
  )
  add_custom_target(package_suite_linux
    COMMAND ${_linux_cmds}
    DEPENDS ${EASY68K_SUITE_DEPS}
    COMMENT "Staging easy68k-qt-Suite for AppImage/DEB packaging")
endif()
