include(${QT_USE_FILE})

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

qt4_add_resources(rfkilltray_RCS
  icons.qrc
)

qt4_wrap_cpp(rfkilltray_CPP
  rftray.h
)

qt4_add_dbus_interfaces(rfkilltray_IFACE
  ${CMAKE_CURRENT_BINARY_DIR}/../service/foo.rfkill.service.xml
  ${CMAKE_CURRENT_BINARY_DIR}/../service/foo.rfkill.device.xml
)

add_executable(rfkilltray
  main.cpp
  rftray.cpp
  ${rfkilltray_CPP}
  ${rfkilltray_RCS}
  ${rfkilltray_IFACE}
)
qt4_use_modules(rfkilltray Core Gui DBus)

install(TARGETS rfkilltray
  RUNTIME DESTINATION bin
)
