
set(PROJECT_NAME chaosircd)
set(PROJECT_VERSION 2.2.1)
set(PROJECT_RELEASE "peace, love & hippie edition")
set(PROJECT_TARNAME ${PROJECT_NAME}-${PROJECT_VERSION})

set(DEFAULT_SHARED_LIB FALSE)

project(${PROJECT_NAME} C)

cmake_minimum_required(VERSION 2.6)

add_subdirectory(libchaos)

set(LEXER_SOURCE src/ircd.l)
set(PARSER_SOURCE src/ircd.y)

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckLibraryExists)

check_include_file(cygwin/in.h HAVE_CYGWIN_IN_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(io.h HAVE_IO_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(netinet/in.h HAVE_NETINET_IN_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(unistd.h HAVE_UNISTD_H)

if(NOT HAVE_UNISTD_H)
  set(YY_NO_UNISTD_H 1)
endif()

if(CMAKE_HOST_WIN32)
#  check_include_file(winsock.h HAVE_WINSOCK2_H)
  check_include_file(winsock2.h HAVE_WINSOCK2_H)
  check_include_file(ws2tcpip.h HAVE_WS2TCPIP_H)
endif(CMAKE_HOST_WIN32)


check_function_exists(select, HAVE_SELECT)


if(HAVE_SYS_SOCKET_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} sys/socket.h)
endif(HAVE_SYS_SOCKET_H)

if(CMAKE_HOST_WIN32)
  if(NOT HAVE_SELECT)
    if(HAVE_WINSOCK2_H)
      set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} winsock2.h)
    endif(HAVE_WINSOCK2_H)
    if(HAVE_WS2TCPIP_H)
      set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} ws2tcpip.h)
    endif(HAVE_WS2TCPIP_H)
    if(HAVE_WINSOCK2_H)
      set(SOCKET_LIBRARIES ws2_32)
    else(HAVE_WINSOCK2_H)
      set(SOCKET_LIBRARIES wsock32)
    endif(HAVE_WINSOCK2_H)
  endif(NOT HAVE_SELECT)
endif(CMAKE_HOST_WIN32)


if(MSVC)
  include_directories(${CMAKE_SOURCE_DIR}/libchaos/include/msinttypes)
  set(HAVE_STDINT_H 1)
  set(HAVE_INTTYPES_H 1)
#  add_definitions(-D_POSIX_)
#  add_definitions( "/W0 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /nologo" )
  add_definitions("/W0 /D_CRT_SECURE_NO_WARNINGS")
  set(MSINTTYPES_HEADERS ${CMAKE_SOURCE_DIR}/libchaos/include/msinttypes/stdint.h  ${CMAKE_SOURCE_DIR}/libchaos/include/msinttypes/inttypes.h)
endif()


set(PREFIX ${CMAKE_INSTALL_PREFIX} CACHE FILEPATH "runtime prefix dir")
set(SYSCONFDIR ${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME} CACHE FILEPATH "configuration directory")
set(PLUGINDIR ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME} CACHE FILEPATH "plugin directory")

add_definitions(-DPREFIX=\"${PREFIX}\")
add_definitions(-DSYSCONFDIR=\"${SYSCONFDIR}\")
add_definitions(-DPLUGINDIR=\"${PLUGINDIR}\")

if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ${DEFAULT_SHARED_LIB})
endif(NOT DEFINED BUILD_SHARED_LIBS)

if(NOT BUILD_SHARED_LIBS)
  add_definitions(-DSTATIC_LIBCHAOS=1)
endif(NOT BUILD_SHARED_LIBS)

add_definitions(-DBUILD_IRCD=1)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/chaosircd/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/include/chaosircd/config.h)

include_directories(
   ${CMAKE_CURRENT_BINARY_DIR}/src
   ${CMAKE_CURRENT_BINARY_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/libchaos/include
   ${CMAKE_CURRENT_BINARY_DIR}/include
)

set(CC "${CMAKE_C_COMPILER}")
set(CFLAGS "${CMAKE_C_FLAGS}")
set(INSTALL "")
set(LIBS "${LINK_LIBRARIES}")
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(bindir "${prefix}/bin")
set(exec_prefix "${prefix}")
set(includedir "${prefix}/include")
set(inidir "${prefix}/var/lib/chaosircd")
set(libexecdir "${prefix}/libexec")
set(logdir "${prefix}/var/log/chaosircd")
set(pidfile "${prefix}/var/run/chaosircd.pid")
set(plugindir "${prefix}/lib/chaosircd")
set(sbindir "${prefix}/sbin")
set(sysconfdir "${prefix}/etc/chaosircd")

set(CONFFILES conf/children.conf conf/classes.conf
   conf/inis.conf conf/ircd.conf conf/logs.conf
   conf/modules.conf conf/opers.conf conf/ssl.conf)

foreach(SUBSTFILE ${CONFFILES})
  message(STATUS "Substituting ${SUBSTFILE}.in")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SUBSTFILE}.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE})
endforeach(SUBSTFILE)

set(TOOLS 
   tools/chaosircd-config tools/mkca tools/mkcrt tools/mkkeys
   tools/mkreq tools/openssl.cnf tools/sign)

foreach(SUBSTFILE ${TOOLS})
  message(STATUS "Substituting ${SUBSTFILE}.in")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SUBSTFILE}.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE} @ONLY NEWLINE_STYLE UNIX)
endforeach(SUBSTFILE)

bison_target(IRCD_PARSER src/ircd.y ${CMAKE_CURRENT_SOURCE_DIR}/src/ircd_parser.c VERBOSE bison.log COMPILE_FLAGS "--defines=${CMAKE_CURRENT_SOURCE_DIR}/src/y.tab.h")
flex_target(IRCD_SCANNER src/ircd.l  ${CMAKE_CURRENT_SOURCE_DIR}/src/ircd_lexer.c)
add_flex_bison_dependency(IRCD_SCANNER IRCD_PARSER)

add_executable(src/chaosircd 
   ${BISON_IRCD_PARSER_OUTPUTS}
   ${FLEX_IRCD_SCANNER_OUTPUTS}
   src/chanmode.c src/channel.c src/chanuser.c src/chars.c
   src/class.c src/client.c src/conf.c src/ircd.c
   src/lclient.c src/main.c src/msg.c src/numeric.c src/oper.c
   src/server.c src/service.c src/user.c src/usermode.c
   ${MSINTTYPES_HEADERS}
   config.h contrib/shellcode.h include/chaosircd/chanmode.h
   include/chaosircd/channel.h include/chaosircd/chanuser.h
   include/chaosircd/chars.h include/chaosircd/class.h
   include/chaosircd/client.h include/chaosircd/conf.h
   include/chaosircd/config.h include/chaosircd/ircd.h
   include/chaosircd/lclient.h include/chaosircd/msg.h
   include/chaosircd/numeric.h include/chaosircd/oper.h
   include/chaosircd/server.h include/chaosircd/service.h
   include/chaosircd/user.h include/chaosircd/usermode.h) 
   
target_link_libraries(src/chaosircd libchaos ${SOCKET_LIBRARIES})

link_directories("${CMAKE_BINARY_DIR}")