set(PACKAGE_NAME libchaos)
set(PACKAGE_VERSION 2.2.1)
set(PACKAGE_RELEASE "peace, love & hippie edition")
set(PACKAGE_TARNAME ${PACKAGE_NAME}-${PACKAGE_VERSION})

set(DEFAULT_SHARED_LIB FALSE)

project(${PACKAGE_NAME} C)

cmake_minimum_required(VERSION 2.6)

set(LIBCHAOS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBCHAOS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformChecks.cmake")

include(FindBISON)
include(FindFLEX)
include(FindOpenSSL)
include(FindFreetype)
include(FindPostgreSQL)


if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ${DEFAULT_SHARED_LIB})
endif(NOT DEFINED BUILD_SHARED_LIBS)

if(NOT BUILD_SHARED_LIBS)
  add_definitions(-DSTATIC_LIBCHAOS=1)
endif(NOT BUILD_SHARED_LIBS)
 
set(DLLEXT "${CMAKE_SHARED_MODULE_SUFFIX}")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SubstituteProjectIncludes.cmake")

substitute_project_includes(${PACKAGE_NAME} "${LIBCHAOS_SOURCE_DIR}" "${LIBCHAOS_BINARY_DIR}")

	       
set(PLUGINDIR ${CMAKE_INSTALL_PREFIX}/lib/${PACKAGE_NAME} CACHE FILEPATH "plugin directory")

add_definitions(-DPLUGINDIR=\"${PLUGINDIR}\")

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

add_subdirectory(test)