set(PACKAGENAME chaosircd)
set(PACKAGEVERSION 2.2.1)
set(PACKAGERELEASE "peace, love & hippie edition")
set(PACKAGETARNAME ${PACKAGENAME}-${PACKAGEVERSION})

set(CMAKE_VERBOSE_MAKEFILE TRUE)

project(${PACKAGENAME} C)

cmake_minimum_required(VERSION 2.6)

set(LIBCHAOS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(LIBCHAOS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib")

add_subdirectory(lib)

execute_process(COMMAND date -R OUTPUT_VARIABLE "CREATION" OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND uname -a OUTPUT_VARIABLE "PLATFORM" OUTPUT_STRIP_TRAILING_WHITESPACE)

  #set(CREATION `date -R`)

  #add_definitions(-DCREATION=\\"${CREATION}\\")


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

include("${CMAKE_SOURCE_DIR}/CMakeModules/FindBISON.cmake")
include("${CMAKE_SOURCE_DIR}/CMakeModules/FindFLEX.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/BuildShared.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/PlatformChecks.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/ConfigBuildType.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/UseOpenSSL.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/UseMySQL.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/UseFreeType.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/UseLibOWFAT.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/DllExt.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/SubstituteProjectIncludes.cmake")

#if(NOT BUILD_SHARED)                                                                                                                                              add_definitions(-DSTATIC_LIBCHAOS=1)
#  add_definitions(-DSTATIC_IRCD=1)
#endif()

if(NOT HAVE_UNISTD_H)
  set(YY_NO_UNISTD_H 1)
endif()

include(CheckIncludeFile)
include(CheckFunctionExists)

check_function_exists(setrlimit HAVE_SETRLIMIT)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(mprotect HAVE_MPROTECT)

add_definitions(-DHAVE_CONFIG_H=1)

check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)

include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/SubstituteProjectIncludes.cmake")

include_project_includes(libchaos "${LIBCHAOS_SOURCE_DIR}" "${LIBCHAOS_BINARY_DIR}")
#substitute_project_includes(ircd "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
substitute_project_includes(ircd "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")

include("${CMAKE_SOURCE_DIR}/CMakeModules/ConfigGNUVars.cmake")



set(CONFFILES
    conf/children.conf
    conf/classes.conf
    conf/inis.conf
    conf/ircd.conf
    conf/logs.conf
    conf/modules.conf
    conf/opers.conf
    conf/ssl.conf)

set(TOOLS
    tools/ircd-config
    tools/mkca
    tools/mkcrt
    tools/mkkeys
    tools/mkreq
    tools/openssl.cnf
    tools/sign)

foreach(SUBSTFILE ${CONFFILES} ${TOOLS})
  message(STATUS "Substituting ${SUBSTFILE}.in")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SUBSTFILE}.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE})
endforeach(SUBSTFILE)

set(CHAOSIRCD_HEADERS
    include/ircd/chanmode.h
    include/ircd/channel.h
    include/ircd/chanuser.h
    include/ircd/chars.h
    include/ircd/class.h
    include/ircd/client.h
    include/ircd/conf.h
    include/ircd/config.h
    include/ircd/ircd.h
    include/ircd/lclient.h
    include/ircd/msg.h
    include/ircd/numeric.h
    include/ircd/oper.h
    include/ircd/server.h
    include/ircd/service.h
    include/ircd/user.h
    include/ircd/usermode.h)

install(FILES ${CHAOSIRCD_HEADERS} DESTINATION include/ircd)
install(FILES ${CONFFILES} DESTINATION etc/chaosircd)
install(FILES ${TOOLS} DESTINATION sbin PERMISSIONS WORLD_EXECUTE)

set(TOOLS 
   tools/ircd-config tools/mkca tools/mkcrt tools/mkkeys
   tools/mkreq tools/openssl.cnf tools/sign)

foreach(SUBSTFILE ${TOOLS})
  message(STATUS "Substituting ${SUBSTFILE}.in")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SUBSTFILE}.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE} @ONLY NEWLINE_STYLE UNIX)
endforeach(SUBSTFILE)

add_subdirectory(src)


add_subdirectory(modules)
