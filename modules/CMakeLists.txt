if(NOT CYGWIN AND NOT WINDOWS)
  set(DEFAULT_SHARED_LIB true)
endif()

include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/BuildShared.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/BuildSharedModules.cmake")

#
# if(NOT BUILD_SHARED) add_definitions(-DSTATIC_LIBCHAOS=1) endif(NOT BUILD_SHARED)
#
# set(PREFIX ${CMAKE_INSTALL_PREFIX} CACHE FILEPATH "runtime prefix dir") set(SYSCONFDIR
# ${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME} CACHE FILEPATH "configuration directory") set(PLUGINDIR
# ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME} CACHE FILEPATH "plugin directory")
#
# add_definitions(-DPREFIX=\"${PREFIX}\") add_definitions(-DSYSCONFDIR=\"${SYSCONFDIR}\")
# add_definitions(-DPLUGINDIR=\"${PLUGINDIR}\")
#
# add_definitions(-DBUILD_IRCD=1)
#
# message(STATUS "Scanning for modules...")

if(BUILD_LOADABLE_MODULES)
  add_definitions(-DBUILD_MODULES=1)
endif()

file(
  GLOB ALL_MODULES
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  */??_*.c */?_*.c)

# lc_lws.c is the only source file in the whole codebase allowed to touch
# libwebsockets (see CMakeLists.txt's BUILD_LC_LWS option) - excluded from
# the glob above by default so a plain build never needs libwebsockets at
# all.
if(NOT BUILD_LC_LWS)
  list(REMOVE_ITEM ALL_MODULES "lclient/lc_lws.c")
endif(NOT BUILD_LC_LWS)

foreach(MODULE_SOURCE ${ALL_MODULES})
  message(STATUS "${MODULE_SOURCE}")

  get_filename_component(MODULE_DIR "${MODULE_SOURCE}" PATH)
  get_filename_component(MODULE_FILE "${MODULE_SOURCE}" NAME)
  get_filename_component(MODULE_NAME "${MODULE_SOURCE}" NAME_WE)

  # string(REGEX REPLACE \\.c$ "" MODULE_NAME "${MODULE_SOURCE}")

  link_directories("${CMAKE_BINARY_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}/.." "${chaosircd_BINARY_DIR}")

  if(BUILD_LOADABLE_MODULES)
    add_library("${MODULE_NAME}" MODULE "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_SOURCE}")
    set_target_properties("${MODULE_NAME}" PROPERTIES PREFIX "" POSITION_INDEPENDENT_CODE ON)

    if(BUILD_LIBIRCD)
      target_link_libraries("${MODULE_NAME}" ircd)
    endif()
    target_link_libraries("${MODULE_NAME}" owfat)

    # target_compile_options("${MODULE_NAME}" BEFORE "-rdynamic ${CMAKE_CURRENT_BINARY_DIR}/../chaosircd")
  else()
    add_library("${MODULE_NAME}" STATIC "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_SOURCE}")
  endif()

  if(MODULE_NAME STREQUAL "lc_lws")
    target_include_directories("${MODULE_NAME}" PRIVATE ${LIBWEBSOCKETS_INCLUDE_DIR})
    target_link_directories("${MODULE_NAME}" PRIVATE ${LIBWEBSOCKETS_LIBRARY_DIR})
    target_link_libraries("${MODULE_NAME}" ${LIBWEBSOCKETS_LIBRARIES})
    add_dependencies("${MODULE_NAME}" libwebsockets)
    target_compile_definitions("${MODULE_NAME}" PRIVATE LC_LWS_GAMJA_DIR="${datadir}/gamja")
  endif(MODULE_NAME STREQUAL "lc_lws")

  install(
    TARGETS "${MODULE_NAME}"
    RUNTIME DESTINATION libexec/chaosircd
    LIBRARY DESTINATION lib/chaosircd
    ARCHIVE DESTINATION lib/chaosircd)
endforeach()
