# Edit68K-Qt (Task 13.19): MDI 68000 editor and integrated assembler IDE.
add_library(easy68k-editor-ui STATIC
  syntax_highlighter.cc
  editor_child.cc
  dialogs.cc
  mainwindow.cc
)
target_include_directories(easy68k-editor-ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(easy68k-editor-ui PRIVATE
  EASY68K_TEMPLATE_PATH="${CMAKE_SOURCE_DIR}/data/template.NEW"
)
target_link_libraries(easy68k-editor-ui PUBLIC
  easym68k-asm
  easy68k-help
  Qt6::Widgets
  Qt6::PrintSupport
)

qt_add_executable(easy68k-editor WIN32 MACOSX_BUNDLE
  main.cc
)
target_link_libraries(easy68k-editor PRIVATE
  easy68k-editor-ui
)
easy68k_configure_gui_app(TARGET easy68k-editor
  BUNDLE_NAME "Edit68K"
  FILE_DESCRIPTION "EASy68K editor and assembler IDE"
)
