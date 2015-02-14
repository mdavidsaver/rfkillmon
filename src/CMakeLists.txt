include(${QT_USE_FILE})

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

qt4_add_resources(rfkilltray_RCS
  icons.qrc
)

qt4_wrap_cpp(rfkilltray_CPP
  tray.h
)

add_executable(rfkilltray
  main.cpp
  dbus.cpp
  tray.cpp
  tray.h
  ${rfkilltray_CPP}
  ${rfkilltray_RCS}
)
qt4_use_modules(rfkilltray Core Gui DBus)

qt4_generate_dbus_interface(tray.h foo.rfkill.xml
  OPTIONS -A
)

add_custom_target(dbusxml ALL DEPENDS foo.rfkill.xml)

install(TARGETS rfkilltray
  RUNTIME DESTINATION bin
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/foo.rfkill.xml
  DESTINATION share/dbus-1/interfaces
)
