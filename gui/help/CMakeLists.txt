# Shared EASy68K Help Viewer (Task 13.25) used by Sim68K-Qt, EASyBIN-Qt, and Edit68K-Qt.
add_library(easy68k-help STATIC
  help_viewer.cc
)
target_include_directories(easy68k-help PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(easy68k-help PRIVATE
  EASY68K_HELP_DIR="${CMAKE_SOURCE_DIR}/data/help"
)
target_link_libraries(easy68k-help PUBLIC
  Qt6::Widgets
)
