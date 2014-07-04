#if(NOT DEFINED BUILD_SHARED_LIBS)
#  set(BUILD_SHARED_LIBS ${DEFAULT_SHARED_LIB})
#endif(NOT DEFINED BUILD_SHARED_LIBS)
#
#if(NOT BUILD_SHARED_LIBS)
#  add_definitions(-DSTATIC_LIBCHAOS=1)
#endif(NOT BUILD_SHARED_LIBS)
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

#add_definitions(-DBUILD_MODULES=1)

file(GLOB ALL_MODULES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" */??_*.c)
link_directories("${CMAKE_CURRENT_BINARY_DIR}/../src" "../src" "${CMAKE_BINARY_DIR}/src")
foreach(MODULE_SOURCE ${ALL_MODULES})
  message(STATUS "${MODULE_SOURCE}")

    get_filename_component(MODULE_DIR "${MODULE_SOURCE}" PATH)
    get_filename_component(MODULE_FILE "${MODULE_SOURCE}" NAME)
    get_filename_component(MODULE_NAME "${MODULE_SOURCE}" NAME_WE)

    #string(REGEX REPLACE \\.c$ "" MODULE_NAME "${MODULE_SOURCE}")

    add_library("${MODULE_NAME}" MODULE "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_SOURCE}")
    set_target_properties("${MODULE_NAME}" PROPERTIES PREFIX "")
    
    target_compile_definitions("${MODULE_NAME}" PUBLIC "-DBUILD_MODULES=1")
    
if(WIN32)
      target_link_libraries("${MODULE_NAME}" ircd)
endif()

    install(TARGETS "${MODULE_NAME}"
            RUNTIME DESTINATION libexec/chaosircd
            LIBRARY DESTINATION lib/chaosircd
            ARCHIVE DESTINATION lib/chaosircd)
endforeach()
