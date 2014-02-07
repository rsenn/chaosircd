set(PACKAGE_NAME chaosircd)
set(PACKAGE_VERSION 2.2.1)
set(PACKAGE_RELEASE "peace, love & hippie edition")
set(PACKAGE_TARNAME ${PACKAGE_NAME}-${PACKAGE_VERSION})

set(DEFAULT_SHARED_LIB FALSE)

project(${PACKAGE_NAME} C)

cmake_minimum_required(VERSION 2.6)

add_subdirectory(libchaos)

set(LEXER_SOURCE src/ircd.l)
set(PARSER_SOURCE src/ircd.y)

include(CheckIncludeFile)

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

set(PREFIX ${CMAKE_INSTALL_PREFIX} CACHE FILEPATH "runtime prefix dir")
set(SYSCONFDIR ${CMAKE_INSTALL_PREFIX}/etc/${PACKAGE_NAME} CACHE FILEPATH "configuration directory")
set(PLUGINDIR ${CMAKE_INSTALL_PREFIX}/lib/${PACKAGE_NAME} CACHE FILEPATH "plugin directory")

add_definitions(-DPREFIX=\"${PREFIX}\")
add_definitions(-DSYSCONFDIR=\"${SYSCONFDIR}\")
add_definitions(-DPLUGINDIR=\"${PLUGINDIR}\")

if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ${DEFAULT_SHARED_LIB})
endif(NOT DEFINED BUILD_SHARED_LIBS)

if(NOT BUILD_SHARED_LIBS)
  add_definitions(-DSTATIC_LIBCHAOS=1)
endif(NOT BUILD_SHARED_LIBS)

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

bison_target(IRCD_PARSER src/ircd.y ${CMAKE_CURRENT_SOURCE_DIR}/src/ircd_parser.c VERBOSE bison.log COMPILE_FLAGS "--defines=${CMAKE_CURRENT_SOURCE_DIR}/src/y.tab.h")
flex_target(IRCD_SCANNER src/ircd.l  ${CMAKE_CURRENT_SOURCE_DIR}/src/ircd_lexer.c)
add_flex_bison_dependency(IRCD_SCANNER IRCD_PARSER)

add_executable(chaosircd 
   ${BISON_IRCD_PARSER_OUTPUTS}
   ${FLEX_IRCD_SCANNER_OUTPUTS}
   src/chanmode.c src/channel.c src/chanuser.c src/chars.c
   src/class.c src/client.c src/conf.c src/ircd.c
   src/lclient.c src/main.c src/msg.c src/numeric.c src/oper.c
   src/server.c src/service.c src/user.c src/usermode.c)
   
 target_link_libraries(chaosircd libchaos)