project(rfkilltray C CXX)

cmake_minimum_required(VERSION 2.8)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

find_package(Qt4 REQUIRED QtCore QtGui QtDBus)

add_subdirectory(service)
add_subdirectory(src)
