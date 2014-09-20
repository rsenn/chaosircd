set(LEXER_SOURCE src/ircd.l)
set(PARSER_SOURCE src/ircd.y)


include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/ConfigSharedLib.cmake")
include("${CMAKE_SOURCE_DIR}/CMakeModules/ConfigGNUVars.cmake")

include_directories("${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

add_definitions(-DBUILD_IRCD=1)

bison_target(IRCD_PARSER ircd.y ${CMAKE_CURRENT_SOURCE_DIR}/ircd_parser.c VERBOSE bison.log COMPILE_FLAGS "--defines=${CMAKE_CURRENT_SOURCE_DIR}/y.tab.h")
flex_target(IRCD_SCANNER ircd.l  ${CMAKE_CURRENT_SOURCE_DIR}/ircd_lexer.c)
add_flex_bison_dependency(IRCD_SCANNER IRCD_PARSER)

add_executable(cgircd 
   ${BISON_IRCD_PARSER_OUTPUTS}
   ${FLEX_IRCD_SCANNER_OUTPUTS}
   chanmode.c channel.c chanuser.c chars.c
   class.c client.c conf.c ircd.c
   lclient.c main.c msg.c numeric.c oper.c
   server.c service.c user.c usermode.c userdb.c
   ${MSINTTYPES_HEADERS}
   "${CMAKE_SOURCE_DIR}/include/ircd/chanmode.h"
   "${CMAKE_SOURCE_DIR}/include/ircd/channel.h" "${CMAKE_SOURCE_DIR}/include/ircd/chanuser.h"
   "${CMAKE_SOURCE_DIR}/include/ircd/chars.h" "${CMAKE_SOURCE_DIR}/include/ircd/class.h"
   "${CMAKE_SOURCE_DIR}/include/ircd/client.h" "${CMAKE_SOURCE_DIR}/include/ircd/conf.h"
   "${CMAKE_SOURCE_DIR}/include/ircd/config.h" "${CMAKE_SOURCE_DIR}/include/ircd/ircd.h"
   "${CMAKE_SOURCE_DIR}/include/ircd/lclient.h" "${CMAKE_SOURCE_DIR}/include/ircd/msg.h"
   "${CMAKE_SOURCE_DIR}/include/ircd/numeric.h" "${CMAKE_SOURCE_DIR}/include/ircd/oper.h"
   "${CMAKE_SOURCE_DIR}/include/ircd/server.h" "${CMAKE_SOURCE_DIR}/include/ircd/service.h"
   "${CMAKE_SOURCE_DIR}/include/ircd/user.h" "${CMAKE_SOURCE_DIR}/include/ircd/usermode.h") 

link_directories("${LIBCHAOS_BINARY_DIR}")
   
target_link_libraries(cgircd libchaos ${SOCKET_LIBRARIES})

install(TARGETS cgircd 
        RUNTIME DESTINATION bin)
