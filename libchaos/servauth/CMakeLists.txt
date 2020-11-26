include("${LIBCHAOS_SOURCE_DIR}/cmake/ConfigSharedLib.cmake")
include("${LIBCHAOS_SOURCE_DIR}/cmake/ConfigGNUVars.cmake")

add_definitions(-DBUILD_SERVAUTH=1)

set(SERVAUTH_SOURCES
    auth.c
    auth.h
    cache.c
    cache.h
    commands.c
    commands.h
    control.c
    control.h
    dns.c
    dns.h
    proxy.c
    proxy.h
    query.c
    query.h
    servauth.c
    servauth.h)
add_executable(servauth ${SERVAUTH_SOURCES})

link_directories("${LIBCHAOS_BINARY_DIR}")

target_link_libraries(servauth libchaos ${SOCKET_LIBRARIES})

install(TARGETS servauth RUNTIME DESTINATION libexec)
