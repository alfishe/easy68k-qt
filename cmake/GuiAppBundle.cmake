# Common configuration for the three GUI applications (Sim68K, Edit68K,
# EASyBIN): macOS bundle properties + icon, Windows exe icon + version info,
# and the compiled-in window icon. Assets live in install/.
#
# Usage (after qt_add_executable(<target> WIN32 MACOSX_BUNDLE ...)):
#   easy68k_configure_gui_app(TARGET <target>
#                             BUNDLE_NAME "Sim68K"
#                             FILE_DESCRIPTION "...")

function(easy68k_configure_gui_app)
  set(oneValueArgs TARGET BUNDLE_NAME FILE_DESCRIPTION)
  cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

  set(app ${ARG_TARGET})
  set_target_properties(${app} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/install/macos/Info.plist"
    MACOSX_BUNDLE_BUNDLE_NAME "${ARG_BUNDLE_NAME}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.easy68k.${app}"
    MACOSX_BUNDLE_ICON_FILE "${app}.icns"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${ARG_BUNDLE_NAME} ${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_INFO_STRING "${ARG_BUNDLE_NAME} ${PROJECT_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "Copyright EASy68K Contributors")

  if(APPLE)
    set(_icns "${CMAKE_SOURCE_DIR}/install/macos/${app}.icns")
    set_source_files_properties(${_icns} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(${app} PRIVATE ${_icns})
  endif()

  if(WIN32)
    set(RC_ICON_PATH "${CMAKE_SOURCE_DIR}/install/windows/${app}.ico")
    set(RC_FILE_DESCRIPTION "${ARG_FILE_DESCRIPTION}")
    set(RC_ORIGINAL_FILENAME "${app}.exe")
    configure_file("${CMAKE_SOURCE_DIR}/install/windows/app.rc.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${app}.rc" @ONLY)
    target_sources(${app} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${app}.rc")
  endif()

  set(_icon_png "${CMAKE_SOURCE_DIR}/install/qrc-icons/${app}.png")
  set_source_files_properties(${_icon_png} PROPERTIES QT_RESOURCE_ALIAS "${app}.png")
  qt_add_resources(${app} "${app}-window-icon"
    PREFIX "/icons"
    FILES ${_icon_png})
endfunction()
