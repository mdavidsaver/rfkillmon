include(${QT_USE_FILE})

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

qt4_wrap_cpp(rfkilldaemon_CPP
  rfdevice.h
  rfservice.h
  nbfile.h
)

add_executable(rfkilldaemon
  main.cpp
  rfdevice.cpp
  rfservice.cpp
  nbfile.cpp
  ${rfkilldaemon_CPP}
)
qt4_use_modules(rfkilldaemon Core Gui DBus)

qt4_generate_dbus_interface(rfservice.h foo.rfkill.service.xml
  OPTIONS -A
)
qt4_generate_dbus_interface(rfdevice.h foo.rfkill.device.xml
  OPTIONS -A
)

add_custom_target(dbusxmladapters ALL
  DEPENDS
    foo.rfkill.service.xml
    foo.rfkill.device.xml
)

install(TARGETS rfkilldaemon
  RUNTIME DESTINATION bin
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/foo.rfkill.service.xml
  ${CMAKE_CURRENT_BINARY_DIR}/foo.rfkill.dbus.xml
  DESTINATION share/dbus-1/interfaces
)
