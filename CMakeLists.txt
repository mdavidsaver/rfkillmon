project(rfkilltray C CXX)

cmake_minimum_required(VERSION 2.8)

find_package(Qt4 REQUIRED QtCore QtGui QtDBus)

add_subdirectory(src)
