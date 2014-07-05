
set(PROJECT_NAME chaosircd)
set(PROJECT_VERSION 2.2.1)
set(PROJECT_RELEASE "peace, love & hippie edition")
set(PROJECT_TARNAME ${PROJECT_NAME}-${PROJECT_VERSION})

set(DEFAULT_SHARED_LIB FALSE)

project(${PROJECT_NAME} C)

cmake_minimum_required(VERSION 2.6)

set(LIBCHAOS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libchaos")
set(LIBCHAOS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/libchaos")

add_subdirectory(libchaos)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

include("${CMAKE_SOURCE_DIR}/CMakeModules/FindBISON.cmake")
include("${CMAKE_SOURCE_DIR}/CMakeModules/FindFLEX.cmake")
include("${CMAKE_SOURCE_DIR}/libchaos/CMakeModules/PlatformChecks.cmake")
include("${CMAKE_SOURCE_DIR}/libchaos/CMakeModules/ConfigBuildType.cmake")

if(NOT HAVE_UNISTD_H)
  set(YY_NO_UNISTD_H 1)
endif()

if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ${DEFAULT_SHARED_LIB})
endif(NOT DEFINED BUILD_SHARED_LIBS)

if(NOT BUILD_SHARED_LIBS)
  add_definitions(-DSTATIC_LIBCHAOS=1)
endif(NOT BUILD_SHARED_LIBS)

include(CheckIncludeFile)

check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)


include("${CMAKE_SOURCE_DIR}/libchaos/CMakeModules/SubstituteProjectIncludes.cmake")

include_project_includes(libchaos "${LIBCHAOS_SOURCE_DIR}" "${LIBCHAOS_BINARY_DIR}")
substitute_project_includes("${PROJECT_NAME}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

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
    tools/chaosircd-config
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
    include/chaosircd/chanmode.h
    include/chaosircd/channel.h
    include/chaosircd/chanuser.h
    include/chaosircd/chars.h
    include/chaosircd/class.h
    include/chaosircd/client.h
    include/chaosircd/conf.h
    include/chaosircd/config.h
    include/chaosircd/ircd.h
    include/chaosircd/lclient.h
    include/chaosircd/msg.h
    include/chaosircd/numeric.h
    include/chaosircd/oper.h
    include/chaosircd/server.h
    include/chaosircd/service.h
    include/chaosircd/user.h
    include/chaosircd/usermode.h)

install(FILES ${CHAOSIRCD_HEADERS} DESTINATION include/chaosircd)
install(FILES ${CONFFILES} DESTINATION etc/chaosircd)
install(FILES ${TOOLS} DESTINATION sbin PERMISSIONS WORLD_EXECUTE)

set(TOOLS 
   tools/chaosircd-config tools/mkca tools/mkcrt tools/mkkeys
   tools/mkreq tools/openssl.cnf tools/sign)

foreach(SUBSTFILE ${TOOLS})
  message(STATUS "Substituting ${SUBSTFILE}.in")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SUBSTFILE}.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE} @ONLY NEWLINE_STYLE UNIX)
endforeach(SUBSTFILE)

add_subdirectory(src)


add_subdirectory(modules)
