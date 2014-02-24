set(LEXER_SOURCE src/ircd.l)
set(PARSER_SOURCE src/ircd.y)

if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ${DEFAULT_SHARED_LIB})
endif(NOT DEFINED BUILD_SHARED_LIBS)

if(NOT BUILD_SHARED_LIBS)
  add_definitions(-DSTATIC_LIBCHAOS=1)
endif(NOT BUILD_SHARED_LIBS)

set(PREFIX ${CMAKE_INSTALL_PREFIX} CACHE FILEPATH "runtime prefix dir")
set(SYSCONFDIR ${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME} CACHE FILEPATH "configuration directory")
set(PLUGINDIR ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME} CACHE FILEPATH "plugin directory")

add_definitions(-DPREFIX=\"${PREFIX}\")
add_definitions(-DSYSCONFDIR=\"${SYSCONFDIR}\")
add_definitions(-DPLUGINDIR=\"${PLUGINDIR}\")

add_definitions(-DBUILD_IRCD=1)

bison_target(IRCD_PARSER ircd.y ${CMAKE_CURRENT_SOURCE_DIR}/ircd_parser.c VERBOSE bison.log COMPILE_FLAGS "--defines=${CMAKE_CURRENT_SOURCE_DIR}/y.tab.h")
flex_target(IRCD_SCANNER ircd.l  ${CMAKE_CURRENT_SOURCE_DIR}/ircd_lexer.c)
add_flex_bison_dependency(IRCD_SCANNER IRCD_PARSER)

add_executable(chaosircd 
   ${BISON_IRCD_PARSER_OUTPUTS}
   ${FLEX_IRCD_SCANNER_OUTPUTS}
   chanmode.c channel.c chanuser.c chars.c
   class.c client.c conf.c ircd.c
   lclient.c main.c msg.c numeric.c oper.c
   server.c service.c user.c usermode.c
   ${MSINTTYPES_HEADERS}
   "${CMAKE_SOURCE_DIR}/include/chaosircd/chanmode.h"
   "${CMAKE_SOURCE_DIR}/include/chaosircd/channel.h" "${CMAKE_SOURCE_DIR}/include/chaosircd/chanuser.h"
   "${CMAKE_SOURCE_DIR}/include/chaosircd/chars.h" "${CMAKE_SOURCE_DIR}/include/chaosircd/class.h"
   "${CMAKE_SOURCE_DIR}/include/chaosircd/client.h" "${CMAKE_SOURCE_DIR}/include/chaosircd/conf.h"
   "${CMAKE_SOURCE_DIR}/include/chaosircd/config.h" "${CMAKE_SOURCE_DIR}/include/chaosircd/ircd.h"
   "${CMAKE_SOURCE_DIR}/include/chaosircd/lclient.h" "${CMAKE_SOURCE_DIR}/include/chaosircd/msg.h"
   "${CMAKE_SOURCE_DIR}/include/chaosircd/numeric.h" "${CMAKE_SOURCE_DIR}/include/chaosircd/oper.h"
   "${CMAKE_SOURCE_DIR}/include/chaosircd/server.h" "${CMAKE_SOURCE_DIR}/include/chaosircd/service.h"
   "${CMAKE_SOURCE_DIR}/include/chaosircd/user.h" "${CMAKE_SOURCE_DIR}/include/chaosircd/usermode.h") 

link_directories("${LIBCHAOS_BINARY_DIR}")
   
target_link_libraries(chaosircd libchaos ${SOCKET_LIBRARIES})
