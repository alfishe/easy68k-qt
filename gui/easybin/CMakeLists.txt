qt_add_executable(easybin
  main.cc
  mainwindow.cc
)
target_link_libraries(easybin PRIVATE
  easym68k-sim
  Qt6::Widgets
)
