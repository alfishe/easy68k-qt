qt_add_executable(easy68k-editor
  main.cc
  mainwindow.cc
  codeeditor.cc
  highlighter.cc
)
target_link_libraries(easy68k-editor PRIVATE
  easym68k-asm
  Qt6::Widgets
)
