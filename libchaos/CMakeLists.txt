set(PACKAGE_NAME libchaos)
set(PACKAGE_VERSION 2.2.1)
set(PACKAGE_RELEASE "peace, love & hippie edition")
set(PACKAGE_TARNAME ${PACKAGE_NAME}-${PACKAGE_VERSION})

set(DEFAULT_SHARED_LIB FALSE)

project(${PACKAGE_NAME} C)

cmake_minimum_required(VERSION 2.6)

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(FindBISON)
include(FindFLEX)
include(FindOpenSSL)
include(FindFreetype)
include(FindPostgreSQL)
include(CheckLibraryExists)
include(CheckFunctionExists)
include(CheckTypeSize)

check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)
check_include_file(sys/poll.h HAVE_SYS_POLL_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(io.h HAVE_IO_H)
check_include_file(netinet/in.h HAVE_NETINET_IN_H)
check_include_file(linux/filter.h HAVE_LINUX_FILTER_H)
check_include_file(linux/types.h HAVE_LINUX_TYPES_H)
check_include_file(net/bpf.h HAVE_NET_BPF_H)
check_include_file(net/ethernet.h HAVE_NET_ETHERNET_H)
check_include_file(cygwin/in.h HAVE_CYGWIN_IN_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(elf.h HAVE_ELF_H)

if(CMAKE_HOST_WIN32)
#  check_include_file(winsock.h HAVE_WINSOCK2_H)
  check_include_file(winsock2.h HAVE_WINSOCK2_H)
  check_include_file(ws2tcpip.h HAVE_WS2TCPIP_H)
endif(CMAKE_HOST_WIN32)

check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)

if(MSVC)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/msinttypes)
  set(HAVE_STDINT_H 1)
  set(HAVE_INTTYPES_H 1)
#  add_definitions(-D_POSIX_)
#  add_definitions( "/W0 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /nologo" )
add_definitions("/W0 /D_CRT_SECURE_NO_WARNINGS")
set(MSINTTYPES_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/msinttypes/stdint.h  ${CMAKE_CURRENT_SOURCE_DIR}/include/msinttypes/inttypes.h)
endif()

check_library_exists(dl dlsym "" HAVE_DLFCN)

if(HAVE_DLFCN)
  set(DLFCN_LIBRARIES dl)
endif()

if(NOT HAVE_DLFCN)
  if(CMAKE_HOST_WIN32)
    set(DLFCN_SOURCES src/dlfcn_win32.c)
  endif(CMAKE_HOST_WIN32)  
endif(NOT HAVE_DLFCN)

check_library_exists(m sin "" HAVE_MATH)

if(HAVE_MATH)
  set(MATH_LIBRARIES m)
endif()


if(HAVE_SYS_SOCKET_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} sys/socket.h)
endif(HAVE_SYS_SOCKET_H)

if(HAVE_WINSOCK2_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} winsock2.h)
endif(HAVE_WINSOCK2_H)

if(HAVE_WS2TCPIP_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} ws2tcpip.h)
endif(HAVE_WS2TCPIP_H)

check_type_size(socklen_t SOCKLEN_T)

check_function_exists(select HAVE_SELECT)


if(CMAKE_HOST_WIN32)
  if(NOT HAVE_SELECT)
    if(HAVE_WINSOCK2_H)
      set(SOCKET_LIBRARIES ws2_32)
    else(HAVE_WINSOCK2_H)
      set(SOCKET_LIBRARIES wsock32)
    endif(HAVE_WINSOCK2_H)
  endif(NOT HAVE_SELECT)
  set(USE_SELECT TRUE)
else(CMAKE_HOST_WIN32)

  check_function_exists(poll HAVE_POLL)

  if(HAVE_POLL)
    set(USE_POLL TRUE)
  elseif(HAVE_SELECT)
    set(USE_SELECT TRUE)
  endif()

endif(CMAKE_HOST_WIN32)


 
set(DLLEXT "${CMAKE_SHARED_MODULE_SUFFIX}")

if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ${DEFAULT_SHARED_LIB})
endif(NOT DEFINED BUILD_SHARED_LIBS)

if(NOT BUILD_SHARED_LIBS)
  add_definitions(-DSTATIC_LIBCHAOS=1)
endif(NOT BUILD_SHARED_LIBS)

if(HAVE_POLL)
  set(USE_POLL 1)
else(HAVE_POLL)
  set(USE_SELECT 1)
endif(HAVE_POLL)

set(PLUGINDIR ${CMAKE_INSTALL_PREFIX}/lib/${PACKAGE_NAME} CACHE FILEPATH "plugin directory")

add_definitions(-DPLUGINDIR=\"${PLUGINDIR}\")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/libchaos
  ${CMAKE_CURRENT_BINARY_DIR}/include)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/libchaos/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/include/libchaos/config.h)
               
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_definitions(-DBUILD_LIBCHAOS=1)

set(LIBCHAOS_HEADERS include/libchaos/cfg.h include/libchaos/child.h
   include/libchaos/connect.h include/libchaos/db.h include/libchaos/defs.h
   include/libchaos/divdi3.h include/libchaos/dlink.h include/libchaos/filter.h
   include/libchaos/font_6x10.h include/libchaos/font_8x13.h
   include/libchaos/font_8x13b.h include/libchaos/gif.h include/libchaos/graph.h
   include/libchaos/hook.h include/libchaos/htmlp.h include/libchaos/httpc.h
   include/libchaos/image.h include/libchaos/image_defpal.h include/libchaos/ini.h
   include/libchaos/io.h include/libchaos/listen.h include/libchaos/log.h
   include/libchaos/mem.h include/libchaos/mfile.h include/libchaos/module.h
   include/libchaos/net.h include/libchaos/queue.h include/libchaos/sauth.h
   include/libchaos/ssl.h include/libchaos/str.h include/libchaos/syscall.h
   include/libchaos/template.h include/libchaos/timer.h include/libchaos/ttf.h
   ${MSINTTYPES_HEADERS})

set(LIBCHAOS_SOURCES src/cfg.c src/child.c src/connect.c src/db.c
   src/divdi3.c src/dlink.c src/filter.c src/gif.c src/graph.c src/hook.c
   src/htmlp.c src/httpc.c src/image.c src/ini.c src/io.c src/listen.c
   src/log.c src/mem.c src/mfile.c src/module.c src/net.c src/queue.c
   src/sauth.c src/ssl.c src/str.c src/syscall.c src/timer.c src/ttf.c ${DLFCN_SOURCES})

add_library(libchaos ${LIBCHAOS_SOURCES} ${LIBCHAOS_HEADERS})

string(REGEX REPLACE \\..* "" LIBRARY_VERSION "${PACKAGE_VERSION}")

set_target_properties(libchaos PROPERTIES SOVERSION ${LIBRARY_VERSION})

target_link_libraries(libchaos LINK_PUBLIC ${SOCKET_LIBRARIES} ${DLFCN_LIBRARIES} ${MATH_LIBRARIES})

add_executable(test/cfgtest test/cfgtest.c)
target_link_libraries(test/cfgtest libchaos)

add_executable(test/dbtest test/dbtest.c)
target_link_libraries(test/dbtest libchaos)

add_executable(test/giftest test/giftest.c)
target_link_libraries(test/giftest libchaos)

add_executable(test/graphtest test/graphtest.c)
target_link_libraries(test/graphtest libchaos)

add_executable(test/imagetest test/imagetest.c)
target_link_libraries(test/imagetest libchaos)

add_executable(test/ttftest test/ttftest.c)
target_link_libraries(test/ttftest libchaos)

