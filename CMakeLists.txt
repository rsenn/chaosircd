set(PACKAGENAME chaosircd)
set(PACKAGEVERSION 2.2.1)
set(PACKAGERELEASE "peace, love & hippie edition")
set(PACKAGETARNAME ${PACKAGENAME}-${PACKAGEVERSION})

set(CMAKE_VERBOSE_MAKEFILE TRUE)

execute_process(
  COMMAND date -R
  OUTPUT_VARIABLE CREATION
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND uname -a
  OUTPUT_VARIABLE PLATFORM
  OUTPUT_STRIP_TRAILING_WHITESPACE)

project(${PACKAGENAME} C)

cmake_minimum_required(VERSION 2.6)

set(SYSCONFDIR
    ${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME}
    CACHE PATH "configuration directory")
set(relsysconfdir
    ${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME}
    CACHE PATH "sysconf dir")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")

set(LIBCHAOS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(LIBCHAOS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib")

include_directories(
  "${CMAKE_CURRENT_BINARY_DIR}" # "${CMAKE_CURRENT_BINARY_DIR}/lib"
)

file(GLOB LIBOWFAT_SOURCES libowfat/*/*.c libowfat/*.h)
add_library(owfat STATIC ${LIBOWFAT_SOURCES})
target_include_directories(owfat PRIVATE libowfat)

add_subdirectory(lib)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

include("${CMAKE_SOURCE_DIR}/CMakeModules/FindBISON.cmake")
include("${CMAKE_SOURCE_DIR}/CMakeModules/FindFLEX.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/BuildShared.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/PlatformChecks.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/ConfigBuildType.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/UseOpenSSL.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/UseMySQL.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/UseFreeType.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/DllExt.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/SubstituteProjectIncludes.cmake")

# if(NOT BUILD_SHARED) add_definitions(-DSTATIC_LIBCHAOS=1)
# add_definitions(-DSTATIC_IRCD=1) endif()

if(NOT HAVE_UNISTD_H)
  set(YY_NO_UNISTD_H 1)
endif()

include(CheckIncludeFile)
include(CheckFunctionExists)

check_function_exists(setrlimit HAVE_SETRLIMIT)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(mprotect HAVE_MPROTECT)

# add_definitions(-std=c89)
add_definitions(-DHAVE_CONFIG_H=1)

check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)

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
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(linux/filter.h HAVE_LINUX_FILTER_H)
check_include_file(linux/types.h HAVE_LINUX_TYPES_H)
check_include_file(net/bpf.h HAVE_NET_BPF_H)
check_include_file(net/ethernet.h HAVE_NET_ETHERNET_H)
check_include_file(elf.h HAVE_ELF_H)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(winsock2.h HAVE_WINSOCK2_H)
check_include_file(ws2tcpip.h HAVE_WS_TCPIP_H)

include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/SubstituteProjectIncludes.cmake")

include_project_includes(libchaos "${LIBCHAOS_SOURCE_DIR}"
                         "${LIBCHAOS_BINARY_DIR}")
# substitute_project_includes(ircd "${CMAKE_CURRENT_SOURCE_DIR}"
# "${CMAKE_CURRENT_BINARY_DIR}")
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
set(INSTALL_CONF_FILES)
foreach(SUBSTFILE ${CONFFILES} ${TOOLS})
  message(STATUS "Substituting ${SUBSTFILE}.in")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SUBSTFILE}.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE})
  list(APPEND INSTALL_CONF_FILES ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE})
endforeach(SUBSTFILE)

set(CHAOSIRCD_HEADERS
    include/ircd/chanmode.h
    include/ircd/channel.h
    include/ircd/chanuser.h
    include/ircd/chars.h
    include/ircd/class.h
    include/ircd/client.h
    include/ircd/conf.h
    ${CMAKE_CURRENT_BINARY_DIR}/include/ircd/config.h
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
install(FILES ${INSTALL_CONF_FILES} DESTINATION etc/${PROJECT_NAME})

set(TOOLS
    tools/ircd-config
    tools/mkca
    tools/mkcrt
    tools/mkkeys
    tools/mkreq
    tools/openssl.cnf
    tools/sign)
set(INSTALL_SUBST_FILES)
foreach(SUBSTFILE ${TOOLS})
  message(STATUS "Substituting ${SUBSTFILE}.in")
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${SUBSTFILE}.in
    ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE} @ONLY NEWLINE_STYLE UNIX)
  list(APPEND INSTALL_SUBST_FILES ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE})
endforeach(SUBSTFILE)

install(
  FILES ${INSTALL_SUBST_FILES}
  DESTINATION sbin
  PERMISSIONS WORLD_EXECUTE)
add_subdirectory(src)

add_subdirectory(modules)
