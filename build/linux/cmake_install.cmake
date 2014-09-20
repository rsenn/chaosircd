# Install script for directory: /home/roman/Sources/cgircd

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "0")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ircd" TYPE FILE FILES
    "/home/roman/Sources/cgircd/include/ircd/chanmode.h"
    "/home/roman/Sources/cgircd/include/ircd/channel.h"
    "/home/roman/Sources/cgircd/include/ircd/chanuser.h"
    "/home/roman/Sources/cgircd/include/ircd/chars.h"
    "/home/roman/Sources/cgircd/include/ircd/class.h"
    "/home/roman/Sources/cgircd/include/ircd/client.h"
    "/home/roman/Sources/cgircd/include/ircd/conf.h"
    "/home/roman/Sources/cgircd/include/ircd/config.h"
    "/home/roman/Sources/cgircd/include/ircd/ircd.h"
    "/home/roman/Sources/cgircd/include/ircd/lclient.h"
    "/home/roman/Sources/cgircd/include/ircd/msg.h"
    "/home/roman/Sources/cgircd/include/ircd/numeric.h"
    "/home/roman/Sources/cgircd/include/ircd/oper.h"
    "/home/roman/Sources/cgircd/include/ircd/server.h"
    "/home/roman/Sources/cgircd/include/ircd/service.h"
    "/home/roman/Sources/cgircd/include/ircd/user.h"
    "/home/roman/Sources/cgircd/include/ircd/usermode.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/etc/cgircd" TYPE FILE FILES
    "/home/roman/Sources/cgircd/conf/children.conf"
    "/home/roman/Sources/cgircd/conf/classes.conf"
    "/home/roman/Sources/cgircd/conf/inis.conf"
    "/home/roman/Sources/cgircd/conf/ircd.conf"
    "/home/roman/Sources/cgircd/conf/logs.conf"
    "/home/roman/Sources/cgircd/conf/modules.conf"
    "/home/roman/Sources/cgircd/conf/opers.conf"
    "/home/roman/Sources/cgircd/conf/ssl.conf"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/sbin" TYPE FILE PERMISSIONS WORLD_EXECUTE FILES
    "/home/roman/Sources/cgircd/tools/ircd-config"
    "/home/roman/Sources/cgircd/tools/mkca"
    "/home/roman/Sources/cgircd/tools/mkcrt"
    "/home/roman/Sources/cgircd/tools/mkkeys"
    "/home/roman/Sources/cgircd/tools/mkreq"
    "/home/roman/Sources/cgircd/tools/openssl.cnf"
    "/home/roman/Sources/cgircd/tools/sign"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  INCLUDE("/home/roman/Sources/cgircd/build/linux/lib/cmake_install.cmake")
  INCLUDE("/home/roman/Sources/cgircd/build/linux/src/cmake_install.cmake")
  INCLUDE("/home/roman/Sources/cgircd/build/linux/modules/cmake_install.cmake")

ENDIF(NOT CMAKE_INSTALL_LOCAL_ONLY)

IF(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
ELSE(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
ENDIF(CMAKE_INSTALL_COMPONENT)

FILE(WRITE "/home/roman/Sources/cgircd/build/linux/${CMAKE_INSTALL_MANIFEST}" "")
FOREACH(file ${CMAKE_INSTALL_MANIFEST_FILES})
  FILE(APPEND "/home/roman/Sources/cgircd/build/linux/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
ENDFOREACH(file)
