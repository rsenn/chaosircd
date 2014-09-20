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

#set(CMAKE_MODULE_PATH "${LIBCHAOS_SOURCE_DIR}/CMakeModules")
#set(CMAKE_CURRENT_LIST_DIR "${LIBCHAOS_SOURCE_DIR}/CMakeModules")

include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/PlatformChecks.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/ConfigBuildType.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/ConfigMathLib.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/FindMySQL.cmake")

if(MYSQL_FOUND)
  add_definitions(-DHAVE_MYSQL=1)
endif()

include(CheckLibraryExists)
include(FindOpenSSL)
include(FindFreetype)
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/FindPostgreSQL.cmake")

check_library_exists(owfat stralloc_init "${CMAKE_INSTALL_PREFIX}/lib" HAVE_LOWFAT)
if(HAVE_LOWFAT)
  set(LOWFAT_LIBRARY owfat)
endif()

add_definitions(-DHAVE_CONFIG_H=1)
include_directories("${MYSQL_INCLUDE_DIR}")

check_type_size(uintptr_t SIZEOF_UINTPTR_T)

if(NOT DEFINED BUILD_SHARED)
  set(BUILD_SHARED "${DEFAULT_SHARED_LIB}" CACHE BOOL "Build dynamically linked binaries")
endif(NOT DEFINED BUILD_SHARED)

if(NOT BUILD_SHARED)
  set(BUILD_STATIC true CACHE BOOL "Build statically linked binaries")

  add_definitions(-DSTATIC_LIBCHAOS=1)
endif(NOT BUILD_SHARED)
 
set(DLLEXT "${CMAKE_SHARED_MODULE_SUFFIX}")

include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/SubstituteProjectIncludes.cmake")

substitute_project_includes(libchaos "${LIBCHAOS_SOURCE_DIR}" "${LIBCHAOS_BINARY_DIR}")

if(LINUX)
  set(DLLEXT ".so" CACHE "shared object extension")
endif(LINUX)
if(WINDOWS)
  set(DLLEXT ".dll" CACHE "shared object extension")
endif(WINDOWS)
if(MAC)
  set(DLLEXT ".dylib" CACHE "shared object extension")
endif(MAC)

set(PLUGINDIR ${CMAKE_INSTALL_PREFIX}/lib/${PACKAGE_NAME} CACHE FILEPATH "plugin directory")

add_definitions(-DDLLEXT=\"${DLLEXT}\" -DPLUGINDIR=\"${PLUGINDIR}\")

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

string(REGEX REPLACE \\..* "" LIBRARY_VERSION "${PACKAGE_VERSION}")

if(BUILD_SHARED)
  add_library(chaos SHARED ${LIBCHAOS_SOURCES} ${LIBCHAOS_HEADERS})

	set_target_properties(chaos PROPERTIES
												SOVERSION "${LIBRARY_VERSION}"
												PREFIX "")

	target_link_libraries(chaos ${MYSQL_LIBRARIES} ${LOWFAT_LIBRARY} ${SOCKET_LIBRARIES} ${DLFCN_LIBRARIES} ${MATH_LIBRARIES})
else()
  add_library(chaos STATIC ${LIBCHAOS_SOURCES} ${LIBCHAOS_HEADERS})

	target_link_libraries(chaos LINK_PUBLIC ${MYSQL_LIBRARIES} ${LOWFAT_LIBRARY} ${SOCKET_LIBRARIES} ${DLFCN_LIBRARIES} ${MATH_LIBRARIES})
endif()

install(TARGETS chaos
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(FILES ${LIBCHAOS_HEADERS} DESTINATION include/libchaos)

add_subdirectory(servauth)
add_subdirectory(test)
