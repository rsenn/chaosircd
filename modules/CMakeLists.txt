if(NOT CYGWIN AND NOT WINDOWS)
  set(DEFAULT_SHARED_LIB true)
endif()

include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/BuildShared.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/BuildSharedModules.cmake")

#
#if(NOT BUILD_SHARED)
#  add_definitions(-DSTATIC_LIBCHAOS=1)
#endif(NOT BUILD_SHARED)
#
#set(PREFIX ${CMAKE_INSTALL_PREFIX} CACHE FILEPATH "runtime prefix dir")
#set(SYSCONFDIR ${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME} CACHE FILEPATH "configuration directory")
#set(PLUGINDIR ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME} CACHE FILEPATH "plugin directory")
#
#add_definitions(-DPREFIX=\"${PREFIX}\")
#add_definitions(-DSYSCONFDIR=\"${SYSCONFDIR}\")
#add_definitions(-DPLUGINDIR=\"${PLUGINDIR}\")
#
#add_definitions(-DBUILD_IRCD=1)
#
#message(STATUS "Scanning for modules...")

if(BUILD_LOADABLE_MODULES)
  add_definitions(-DBUILD_MODULES=1)
endif()

file(GLOB ALL_MODULES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" */??_*.c)

foreach(MODULE_SOURCE ${ALL_MODULES})
  message(STATUS "${MODULE_SOURCE}")

    get_filename_component(MODULE_DIR "${MODULE_SOURCE}" PATH)
    get_filename_component(MODULE_FILE "${MODULE_SOURCE}" NAME)
    get_filename_component(MODULE_NAME "${MODULE_SOURCE}" NAME_WE)

    #string(REGEX REPLACE \\.c$ "" MODULE_NAME "${MODULE_SOURCE}")

    link_directories("${CMAKE_BINARY_DIR}/src" "${cgircd_BINARY_DIR}")

    if(BUILD_LOADABLE_MODULES)
      add_library("${MODULE_NAME}" MODULE SHARED "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_SOURCE}")
      set_target_properties("${MODULE_NAME}" PROPERTIES PREFIX "")
      
      if(WINDOWS OR CYGWIN)
          target_link_libraries("${MODULE_NAME}" ircd)
      endif()

      #      target_compile_options("${MODULE_NAME}" BEFORE "-rdynamic ${CMAKE_CURRENT_BINARY_DIR}/../cgircd")
    else()
      add_library("${MODULE_NAME}" STATIC "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_SOURCE}")
    endif()

    install(TARGETS "${MODULE_NAME}"
            RUNTIME DESTINATION libexec/cgircd
            LIBRARY DESTINATION lib/cgircd
            ARCHIVE DESTINATION lib/cgircd)
endforeach()
