# EASyBIN-Qt (Task 13.20): binary file creation and editing utility.
add_library(easy68k-easybin-ui STATIC
  mainwindow.cc
)
target_include_directories(easy68k-easybin-ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(easy68k-easybin-ui PUBLIC
  easym68k-sim
  Qt6::Widgets
)

qt_add_executable(easybin
  main.cc
)
target_link_libraries(easybin PRIVATE
  easy68k-easybin-ui
)
