set(PACKAGE_NAME libchaos)
set(PACKAGE_VERSION 2.2.1)
set(PACKAGE_RELEASE "peace, love & hippie edition")
set(PACKAGE_TARNAME ${PACKAGE_NAME}-${PACKAGE_VERSION})

if(WINDOWS OR CYGWIN)
  set(DEFAULT_SHARED_LIB TRUE)
else()
  set(DEFAULT_SHARED_LIB FALSE)
endif()

project(${PACKAGE_NAME} C)

cmake_minimum_required(VERSION 2.6)

if(NOT LIBCHAOS_SOURCE_DIR)
  set(LIBCHAOS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
if(NOT LIBCHAOS_BINARY_DIR)
  set(LIBCHAOS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

# set(CMAKE_MODULE_PATH "${LIBCHAOS_SOURCE_DIR}/CMakeModules")
# set(CMAKE_CURRENT_LIST_DIR "${LIBCHAOS_SOURCE_DIR}/CMakeModules")

include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/PlatformChecks.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/ConfigBuildType.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/BuildShared.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/DllExt.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/ConfigMathLib.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/UseOpenSSL.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/UseMySQL.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/UseFreeType.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/FindPostgreSQL.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/SubstituteProjectIncludes.cmake")

if(BUILD_STATIC)
  add_definitions(-DSTATIC_LIBCHAOS=1)
endif()

# add_definitions(-std=c89)

include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckFunctionExists)

check_include_file(cygwin/in.h HAVE_CYGWIN_IN_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(elf.h HAVE_ELF_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(linux/filter.h HAVE_LINUX_FILTER_H)
check_include_file(linux/types.h HAVE_LINUX_TYPES_H)
check_include_file(net/bpf.h HAVE_NET_BPF_H)
check_include_file(net/ethernet.h HAVE_NET_ETHERNET_H)
check_include_file(netinet/in.h HAVE_NETINET_IN_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/poll.h HAVE_SYS_POLL_H)
check_include_file(winsock2.h HAVE_WINSOCK2_H)
check_include_file(ws2tcpip.h HAVE_WS_TCPIP_H)
check_include_file(unistd.h HAVE_UNISTD_H)

check_function_exists(select HAVE_SELECT)
check_function_exists(poll HAVE_POLL)
check_function_exists(socketpair HAVE_SOCKETPAIR)

if(HAVE_POLL)
  set(USE_POLL 1)
else(HAVE_POLL)
  set(USE_SELECT 1)
endif(HAVE_POLL)

check_library_exists(owfat stralloc_init "${CMAKE_INSTALL_PREFIX}/lib"
                     HAVE_LOWFAT)
if(HAVE_LOWFAT)
  set(LOWFAT_LIBRARY owfat)
endif()

add_definitions(-DHAVE_CONFIG_H=1)

check_type_size(uintptr_t SIZEOF_UINTPTR_T)

substitute_project_includes(libchaos "${LIBCHAOS_SOURCE_DIR}"
                            "${LIBCHAOS_BINARY_DIR}")

set(PLUGINDIR
    ${CMAKE_INSTALL_PREFIX}/lib/${PACKAGE_NAME}
    CACHE FILEPATH "plugin directory")

add_definitions(-DPLUGINDIR=\"${PLUGINDIR}\")

add_definitions(-DBUILD_LIBCHAOS=1)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(LIBCHAOS_HEADERS
    include/libchaos/cfg.h
    include/libchaos/child.h
    include/libchaos/connect.h
    include/libchaos/db.h
    include/libchaos/defs.h
    include/libchaos/divdi3.h
    include/libchaos/dlink.h
    include/libchaos/filter.h
    include/libchaos/font_6x10.h
    include/libchaos/font_8x13.h
    include/libchaos/font_8x13b.h
    include/libchaos/gif.h
    include/libchaos/graph.h
    include/libchaos/hook.h
    include/libchaos/htmlp.h
    include/libchaos/httpc.h
    include/libchaos/image.h
    include/libchaos/image_defpal.h
    include/libchaos/ini.h
    include/libchaos/io.h
    include/libchaos/listen.h
    include/libchaos/log.h
    include/libchaos/mem.h
    include/libchaos/mfile.h
    include/libchaos/module.h
    include/libchaos/net.h
    include/libchaos/queue.h
    include/libchaos/sauth.h
    include/libchaos/ssl.h
    include/libchaos/str.h
    include/libchaos/syscall.h
    include/libchaos/template.h
    include/libchaos/timer.h
    include/libchaos/ttf.h
    ${MSINTTYPES_HEADERS})

set(LIBCHAOS_SOURCES
    src/cfg.c
    src/child.c
    src/connect.c
    src/db.c
    src/divdi3.c
    src/dlink.c
    src/filter.c
    src/gif.c
    src/graph.c
    src/hook.c
    src/htmlp.c
    src/httpc.c
    src/image.c
    src/ini.c
    src/io.c
    src/listen.c
    src/log.c
    src/mem.c
    src/mfile.c
    src/module.c
    src/net.c
    src/queue.c
    src/sauth.c
    src/ssl.c
    src/str.c
    src/syscall.c
    src/timer.c
    src/ttf.c
    ${DLFCN_SOURCES})

string(REGEX REPLACE \\..* "" LIBRARY_VERSION "${PACKAGE_VERSION}")

if(BUILD_SHARED)
  add_library(chaos SHARED ${LIBCHAOS_SOURCES} ${LIBCHAOS_HEADERS})

  set_target_properties(
    chaos PROPERTIES SOVERSION "${LIBRARY_VERSION}"
                     INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

  target_link_libraries(
    chaos
    ${OPENSSL_LIBS}
    ${MYSQL_LIBS}
    ${FREETYPE_LIBS}
    ${LOWFAT_LIBRARY}
    ${SOCKET_LIBRARIES}
    ${DLFCN_LIBRARIES}
    ${MATH_LIBRARIES})
else()
  add_library(chaos STATIC ${LIBCHAOS_SOURCES} ${LIBCHAOS_HEADERS})

  target_link_libraries(
    chaos
    LINK_PUBLIC
    ${OPENSSL_LIBS}
    ${MYSQL_LIBS}
    ${FREETYPE_LIBS}
    ${LOWFAT_LIBRARY}
    ${SOCKET_LIBRARIES}
    ${DLFCN_LIBRARIES}
    ${MATH_LIBRARIES})
endif()

install(
  TARGETS chaos
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(FILES ${LIBCHAOS_HEADERS} DESTINATION include/libchaos)

add_subdirectory(servauth)
add_subdirectory(test)
