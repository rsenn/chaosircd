# - Find MySQL
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MYSQL_FOUND, If false, do not try to use MySQL.
#
# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
   set(MYSQL_FOUND TRUE)

else(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

  find_path(MYSQL_INCLUDE_DIR mysql.h
      /usr/include/mysql
      /usr/local/include/mysql
      $ENV{ProgramFiles}/MySQL/*/include
      $ENV{SystemDrive}/MySQL/*/include
      )

  find_library(MYSQL_LIBRARIES NAMES mysqlclient
      PATHS
      /usr/lib/mysql
      /usr/local/lib/mysql
      $ENV{ProgramFiles}/MySQL/*/lib/opt
      $ENV{SystemDrive}/MySQL/*/lib/opt
      )

  if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
    set(MYSQL_FOUND TRUE)
    message(STATUS "Found MySQL: ${MYSQL_INCLUDE_DIR}, ${MYSQL_LIBRARIES}")
  else(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
    set(MYSQL_FOUND FALSE)
    message(STATUS "MySQL not found.")
  endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

  mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)

endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)	

if(MYSQL_FOUND)
  set(USE_MYSQL true CACHE BOOL "Use MySQL")
endif()

if(USE_MYSQL)
  set(MYSQL_LIBS "${MYSQL_LIBRARIES}")
  include_directories("${MYSQL_INCLUDE_DIR}")
      #  add_definitions(-DHAVE_MYSQL=1)
endif()


