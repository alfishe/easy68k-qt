# Trap #15 Qt back ends (Task 13.1): the I/O window model and the adapters
# implementing the simulator's I/O interfaces. No widgets: usable headless
# (QT_QPA_PLATFORM=offscreen) by the tests.
add_library(easy68k-simio STATIC
  key_map.cc
  sim_io_screen.cc
  qt_text_io.cc
  qt_graphics_io.cc
  qt_peripheral_io.cc
  qt_simulator_env.cc
  qt_file_io.cc
  qt_print_io.cc
  qt_sound_io.cc
  qt_serial_io.cc
  listing.cc
  sim_session.cc
  sim_log.cc
)
target_include_directories(easy68k-simio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(easy68k-simio PUBLIC easym68k-sim Qt6::Gui Qt6::Multimedia)
if(Qt6SerialPort_FOUND)
  target_link_libraries(easy68k-simio PUBLIC Qt6::SerialPort)
  target_compile_definitions(easy68k-simio PUBLIC EASYM68K_HAS_SERIALPORT=1)
endif()

# Sim68K-Qt widgets (Task 13.2-): main window, register panel, listing,
# I/O window. A library so the tests can drive the main window.
add_library(easy68k-simulator-ui STATIC
  io_window.cc
  register_widget.cc
  source_view.cc
  stack_window.cc
  memory_window.cc
  hardware_window.cc
  breakpoints_window.cc
  fullscreen_options_dialog.cc
  log_window.cc
  mainwindow.cc
)
target_include_directories(easy68k-simulator-ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(easy68k-simulator-ui PUBLIC easy68k-simio Qt6::Widgets Qt6::PrintSupport)
# hardwareu.dfm glyphs: toggle switch (off | on), push button, IRQ/Reset
# button (up | disabled).
qt_add_resources(easy68k-simulator-ui hardware_glyphs
  PREFIX /hardware
  BASE resources
  FILES resources/switch.png resources/push_button.png resources/irq_button.png
)

qt_add_executable(easy68k-simulator main.cc)
target_link_libraries(easy68k-simulator PRIVATE easy68k-simulator-ui)
