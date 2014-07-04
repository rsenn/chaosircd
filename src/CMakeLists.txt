set(LEXER_SOURCE src/ircd.l)
set(PARSER_SOURCE src/ircd.y)


include("${LIBCHAOS_SOURCE_DIR}/cmake/ConfigSharedLib.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/ConfigGNUVars.cmake")


bison_target(IRCD_PARSER ircd.y ${CMAKE_CURRENT_SOURCE_DIR}/ircd_parser.c VERBOSE bison.log COMPILE_FLAGS "--defines=${CMAKE_CURRENT_SOURCE_DIR}/y.tab.h")
flex_target(IRCD_SCANNER ircd.l  ${CMAKE_CURRENT_SOURCE_DIR}/ircd_lexer.c)
add_flex_bison_dependency(IRCD_SCANNER IRCD_PARSER)

set(SOURCES
${BISON_IRCD_PARSER_OUTPUTS}
   ${FLEX_IRCD_SCANNER_OUTPUTS}
   chanmode.c channel.c chanuser.c chars.c
   class.c client.c conf.c
   lclient.c msg.c numeric.c oper.c ircd.c
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

if(WIN32)

add_library(ircd SHARED ${SOURCES})
 set_target_properties(ircd PROPERTIES PREFIX "")
target_compile_definitions(ircd PUBLIC "-DBUILD_IRCD=1")

add_executable(chaosircd main.c "${CMAKE_SOURCE_DIR}/include/chaosircd/ircd.h" "${CMAKE_SOURCE_DIR}/include/chaosircd/config.h" "${CMAKE_SOURCE_DIR}/include/chaosircd/ircd.h")
target_link_libraries(ircd libchaos ${SOCKET_LIBRARIES})

target_link_libraries(chaosircd ircd)

else(WIN32)

add_executable(chaosircd ${SOURCES} main.c)
    

link_directories("${LIBCHAOS_BINARY_DIR}")
   
target_link_libraries(chaosircd libchaos ${SOCKET_LIBRARIES})
endif(WIN32)

install(TARGETS chaosircd 
        RUNTIME DESTINATION bin)
