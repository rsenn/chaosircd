set(LEXER_SOURCE src/ircd.l)
set(PARSER_SOURCE src/ircd.y)

include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/ConfigSharedLib.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/UseMySQL.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/UseFreeType.cmake")
include("${CMAKE_SOURCE_DIR}/CMakeModules/ConfigGNUVars.cmake")

bison_target(
  IRCD_PARSER ircd.y ${CMAKE_CURRENT_SOURCE_DIR}/ircd_parser.c
  VERBOSE bison.log
  COMPILE_FLAGS "--defines=${CMAKE_CURRENT_SOURCE_DIR}/y.tab.h")
flex_target(IRCD_SCANNER ircd.l ${CMAKE_CURRENT_SOURCE_DIR}/ircd_lexer.c)
add_flex_bison_dependency(IRCD_SCANNER IRCD_PARSER)

set(SOURCES
    ${BISON_IRCD_PARSER_OUTPUTS}
    ${FLEX_IRCD_SCANNER_OUTPUTS}
    chanmode.c
    channel.c
    chanuser.c
    chars.c
    class.c
    client.c
    conf.c
    lclient.c
    msg.c
    numeric.c
    oper.c
    ircd.c
    server.c
    service.c
    user.c
    usermode.c
    ${MSINTTYPES_HEADERS}
    "${CMAKE_SOURCE_DIR}/include/ircd/chanmode.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/channel.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/chanuser.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/chars.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/class.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/client.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/conf.h"
    "${CMAKE_BINARY_DIR}/include/ircd/config.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/ircd.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/lclient.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/msg.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/numeric.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/oper.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/server.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/service.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/user.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/usermode.h")


  add_definitions(-DHAVE_SSL=1)
if(WINDOWS OR CYGWIN)

  add_library(ircd SHARED ${SOURCES})
  set_target_properties(ircd PROPERTIES INSTALL_RPATH
                                        "${CMAKE_INSTALL_PREFIX}/lib")
  target_compile_definitions(ircd PUBLIC "-DBUILD_IRCD=1")

  add_executable(
    cgircd
    main.c "${CMAKE_SOURCE_DIR}/include/ircd/ircd.h"
    "${CMAKE_BINARY_DIR}/include/ircd/config.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/ircd.h")
  set_target_properties(cgircd PROPERTIES INSTALL_RPATH
                                          "${CMAKE_INSTALL_PREFIX}/lib")

  link_directories("${CMAKE_CURRENT_BINARY_DIR}/../lib"
                   "${LIBCHAOS_BINARY_DIR}")

  target_link_libraries(ircd chaos ${SOCKET_LIBRARIES})
  target_link_libraries(cgircd ircd)

else()
  link_directories("${CMAKE_CURRENT_BINARY_DIR}/../lib")
  add_executable(cgircd ${SOURCES} main.c)

  link_directories("${LIBCHAOS_BINARY_DIR}")

  target_link_libraries(cgircd chaos ${MYSQL_LIBS} ${OPENSSL_LIBS}
                        ${SOCKET_LIBRARIES})
endif()

install(TARGETS cgircd RUNTIME DESTINATION bin)
