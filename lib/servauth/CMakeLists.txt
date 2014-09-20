include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/ConfigSharedLib.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/ConfigGNUVars.cmake")

add_definitions(-DBUILD_SERVAUTH=1)

set(SERVAUTH_SOURCES auth.c auth.h cache.c cache.h commands.c commands.h control.c control.h dns.c dns.h proxy.c proxy.h query.c query.h userdb.c userdb.h servauth.c servauth.h)
add_executable(servauth ${SERVAUTH_SOURCES})

link_directories("${LIBCHAOS_BINARY_DIR}")
   
target_link_libraries(servauth libchaos ${MYSQL_LIBRARIES} ${OPENSSL_SSL_LIBRARY} ${SOCKET_LIBRARIES})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}"/include "${CMAKE_CURRENT_SOURCE_DIR}/.." "${CMAKE_CURRENT_SOURCE_DIR}/../include" "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include")

install(TARGETS servauth 
        RUNTIME DESTINATION libexec)
