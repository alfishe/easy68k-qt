# EASyBIN-Qt (Task 13.20): binary file creation and editing utility.
add_library(easy68k-easybin-ui STATIC
  mainwindow.cc
)
target_include_directories(easy68k-easybin-ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(easy68k-easybin-ui PUBLIC
  easym68k-sim
  easy68k-help
  Qt6::Widgets
)

qt_add_executable(easybin WIN32 MACOSX_BUNDLE
  main.cc
)
target_link_libraries(easybin PRIVATE
  easy68k-easybin-ui
)
easy68k_configure_gui_app(TARGET easybin
  BUNDLE_NAME "EASyBIN"
  FILE_DESCRIPTION "EASy68K binary file editor"
)
