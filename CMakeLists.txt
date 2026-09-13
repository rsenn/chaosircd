cmake_minimum_required(VERSION 3.5)

set(PACKAGENAME chaosircd)
set(PACKAGEVERSION 2.2.1)
set(PACKAGERELEASE "peace, love & hippie edition")
set(PACKAGETARNAME ${PACKAGENAME}-${PACKAGEVERSION})

execute_process(
  COMMAND date -R
  OUTPUT_VARIABLE CREATION
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND uname -a
  OUTPUT_VARIABLE PLATFORM
  OUTPUT_STRIP_TRAILING_WHITESPACE)

project(${PACKAGENAME} C)

if(WIN32 OR CYGWIN)
  option(BUILD_LIBIRCD "Build ircd as library" ON)
else(WIN32 OR CYGWIN)
  option(BUILD_LIBIRCD "Build ircd as library" OFF)
endif(WIN32 OR CYGWIN)

set(SYSCONFDIR
    ${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME}
    CACHE PATH "configuration directory")
set(relsysconfdir
    ${SYSCONFDIR}
    CACHE PATH "sysconf dir (defaults to \${SYSCONFDIR} - override that instead unless you need them to differ)")
# Pin a C standard predating C23's built-in true/false/bool keywords -
# include/ircd/ircd.h defines its own `typedef enum { false, true } bool;`,
# which is a hard error under -std=*c23 (the GNU default on newer GCC).
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -std=gnu17")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(LIBCHAOS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(LIBCHAOS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib")

include_directories(
  "${CMAKE_CURRENT_BINARY_DIR}" # "${CMAKE_CURRENT_BINARY_DIR}/lib"
  "${CMAKE_CURRENT_SOURCE_DIR}" # "${CMAKE_CURRENT_SOURCE_DIR}/libowfat"
)

file(
  GLOB
  LIBOWFAT_SOURCES
  libowfat/byte/*.c
  libowfat/str/*.c
  libowfat/stralloc/*.c
  libowfat/uint/*.c
  libowfat/*.h)

add_library(owfat STATIC ${LIBOWFAT_SOURCES})
target_include_directories(owfat PRIVATE libowfat)

add_subdirectory(lib)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

include("${CMAKE_SOURCE_DIR}/CMakeModules/FindBISON.cmake")
include("${CMAKE_SOURCE_DIR}/CMakeModules/FindFLEX.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/BuildShared.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/PlatformChecks.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/ConfigBuildType.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/UseOpenSSL.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/UseMySQL.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/UseFreeType.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/DllExt.cmake")
include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/SubstituteProjectIncludes.cmake")

# if(NOT BUILD_SHARED) add_definitions(-DSTATIC_LIBCHAOS=1) add_definitions(-DSTATIC_IRCD=1) endif()

if(NOT HAVE_UNISTD_H)
  set(YY_NO_UNISTD_H 1)
endif()

include(CheckIncludeFile)
include(CheckFunctionExists)

check_function_exists(setrlimit HAVE_SETRLIMIT)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(mprotect HAVE_MPROTECT)

# add_definitions(-std=c89)
add_definitions(-DHAVE_CONFIG_H=1)

check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)

check_include_file(cygwin/in.h HAVE_CYGWIN_IN_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(io.h HAVE_IO_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(netinet/in.h HAVE_NETINET_IN_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(linux/filter.h HAVE_LINUX_FILTER_H)
check_include_file(linux/types.h HAVE_LINUX_TYPES_H)
check_include_file(net/bpf.h HAVE_NET_BPF_H)
check_include_file(net/ethernet.h HAVE_NET_ETHERNET_H)
check_include_file(elf.h HAVE_ELF_H)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(winsock2.h HAVE_WINSOCK2_H)
check_include_file(ws2tcpip.h HAVE_WS_TCPIP_H)

include("${CMAKE_SOURCE_DIR}/lib/CMakeModules/SubstituteProjectIncludes.cmake")

include_project_includes(libchaos "${LIBCHAOS_SOURCE_DIR}" "${LIBCHAOS_BINARY_DIR}")
# substitute_project_includes(ircd "${CMAKE_CURRENT_SOURCE_DIR}"
# "${CMAKE_CURRENT_BINARY_DIR}")
substitute_project_includes(ircd "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")

include("${CMAKE_SOURCE_DIR}/CMakeModules/ConfigGNUVars.cmake")

set(CONFFILES
    conf/children.conf
    conf/classes.conf
    conf/inis.conf
    conf/ircd.conf
    conf/logs.conf
    conf/modules.conf
    conf/opers.conf
    conf/ssl.conf)

set(INSTALL_CONF_FILES)
foreach(SUBSTFILE ${CONFFILES})
  message(STATUS "Substituting ${SUBSTFILE}.in")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SUBSTFILE}.in ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE})
  list(APPEND INSTALL_CONF_FILES ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE})
endforeach(SUBSTFILE)

set(CHAOSIRCD_HEADERS
    include/ircd/chanmode.h
    include/ircd/channel.h
    include/ircd/chanuser.h
    include/ircd/chars.h
    include/ircd/class.h
    include/ircd/client.h
    include/ircd/conf.h
    ${CMAKE_CURRENT_BINARY_DIR}/include/ircd/config.h
    include/ircd/ircd.h
    include/ircd/lclient.h
    include/ircd/msg.h
    include/ircd/numeric.h
    include/ircd/oper.h
    include/ircd/server.h
    include/ircd/service.h
    include/ircd/user.h
    include/ircd/usermode.h)

install(FILES ${CHAOSIRCD_HEADERS} DESTINATION include/ircd)
install(FILES ${INSTALL_CONF_FILES} DESTINATION "${sysconfdir}")

# runtime directories referenced by the conf/*.conf templates (inidir,
# logdir) - created empty so a fresh install doesn't need them hand-made,
# and so ini/log paths don't fall back to "/" (see relinidir/rellogdir in
# CMakeModules/ConfigGNUVars.cmake). ${inidir}/${logdir} are absolute
# paths (CACHE PATH, default under CMAKE_INSTALL_PREFIX but overridable
# to real FHS locations, e.g. by debian/rules), so install() uses them
# as-is rather than as CMAKE_INSTALL_PREFIX-relative destinations.
install(DIRECTORY DESTINATION "${inidir}")
install(DIRECTORY DESTINATION "${logdir}")

set(TOOLS
    tools/configure-server
    tools/ircd-config
    tools/mkca
    tools/mkcrt
    tools/mkkeys
    tools/mkreq
    tools/openssl.cnf
    tools/sign)
set(INSTALL_SUBST_FILES)
foreach(SUBSTFILE ${TOOLS})
  message(STATUS "Substituting ${SUBSTFILE}.in")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SUBSTFILE}.in ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE} @ONLY
                 NEWLINE_STYLE UNIX)
  list(APPEND INSTALL_SUBST_FILES ${CMAKE_CURRENT_BINARY_DIR}/${SUBSTFILE})
endforeach(SUBSTFILE)

install(
  FILES ${INSTALL_SUBST_FILES}
  DESTINATION "${sbindir}"
  PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE)

# runit service definition. Where it lands is overridable (e.g. Debian
# packaging installs it at the runit-services convention /etc/sv/<name>
# instead of under the runtime prefix) - it is NOT linked into a runsvdir
# automatically either way, do that manually:
#
#   - Everywhere else: ln -s <runitservicedir> /etc/service/chaosircd
#   - Termux (termux-services package): the service definition belongs
#     under etc/sv, not etc/service - termux-services' runsvdir watches
#     $PREFIX/var/service, and `sv-enable chaosircd` (or a manual
#     `ln -s <runitservicedir> $PREFIX/var/service/chaosircd`) is what
#     actually starts it. Detected here via $TERMUX_VERSION, which
#     Termux's own shell environment always sets; override
#     -Drunitservicedir=... directly if that heuristic ever misses.
#
# Either way this is CMAKE_INSTALL_PREFIX-relative (see ${prefix} above),
# so building with -DCMAKE_INSTALL_PREFIX=$PREFIX (Termux's own, e.g.
# /data/data/com.termux/files/usr - not /usr/local) is all that is needed
# for the rest of the install (binaries, config, modules, ...) to land in
# the right place too.
if(DEFINED ENV{TERMUX_VERSION})
  set(runitservicesdir_default "${prefix}/etc/sv")
else()
  set(runitservicesdir_default "${prefix}/etc/service")
endif()

set(runitservicedir
    "${runitservicesdir_default}/${PROJECT_NAME}"
    CACHE PATH "installed runit service directory")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/service/chaosircd/run.in ${CMAKE_CURRENT_BINARY_DIR}/service/chaosircd/run
               @ONLY NEWLINE_STYLE UNIX)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/service/chaosircd/log/run.in
               ${CMAKE_CURRENT_BINARY_DIR}/service/chaosircd/log/run @ONLY NEWLINE_STYLE UNIX)

set(SERVICE_EXEC_PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/service/chaosircd/run
  DESTINATION "${runitservicedir}"
  PERMISSIONS ${SERVICE_EXEC_PERMISSIONS})
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/service/chaosircd/log/run
  DESTINATION "${runitservicedir}/log"
  PERMISSIONS ${SERVICE_EXEC_PERMISSIONS})

# WebSocket<->IRC gateway (tools/ws-irc-gateway.js, a QuickJS script using
# the 'lws' module - see that file for why) - lets browser IRC clients
# (which can only open ws://+wss:// sockets, not raw TCP) reach chaosircd.
# Not linked into a runsvdir automatically, same as chaosircd's own
# service above (including the Termux etc/sv vs etc/service split):
#   ln -s <ws_irc_gateway_servicedir> /etc/service/ws-irc-gateway
set(ws_irc_gateway_servicedir
    "${runitservicesdir_default}/ws-irc-gateway"
    CACHE PATH "installed ws-irc-gateway runit service directory")

install(FILES tools/ws-irc-gateway.js DESTINATION "${libexecdir}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/service/ws-irc-gateway/run.in
               ${CMAKE_CURRENT_BINARY_DIR}/service/ws-irc-gateway/run @ONLY NEWLINE_STYLE UNIX)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/service/ws-irc-gateway/log/run.in
               ${CMAKE_CURRENT_BINARY_DIR}/service/ws-irc-gateway/log/run @ONLY NEWLINE_STYLE UNIX)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/service/ws-irc-gateway/run
  DESTINATION "${ws_irc_gateway_servicedir}"
  PERMISSIONS ${SERVICE_EXEC_PERMISSIONS})
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/service/ws-irc-gateway/log/run
  DESTINATION "${ws_irc_gateway_servicedir}/log"
  PERMISSIONS ${SERVICE_EXEC_PERMISSIONS})

# lc_lws (modules/lclient/lc_lws.c): in-process WebSocket<->IRC gateway
# module. Off by default - it needs libwebsockets, which nothing else here
# depends on and which chaosircd does not vendor - turning this on makes
# CMake `git clone` upstream libwebsockets into 3rdparty/libwebsockets (if
# not already there) and build it as a static lib via
# CMakeModules/BuildLibwebsockets.cmake (adapted from qjs-lws's copy),
# with -DLWS_WITH_EXTERNAL_POLL:BOOL=ON so lc_lws can drive it from
# libchaos's own io_poll()/io_select() instead of lws running its own
# event loop.
option(BUILD_LC_LWS "Build lc_lws, the in-process WebSocket<->IRC gateway module (needs libwebsockets)" OFF)

if(BUILD_LC_LWS)
  set(LIBWEBSOCKETS_SOURCE_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libwebsockets"
      CACHE PATH "libwebsockets source directory")

  if(NOT EXISTS "${LIBWEBSOCKETS_SOURCE_DIR}/CMakeLists.txt")
    find_package(Git REQUIRED)

    message(STATUS "Cloning libwebsockets into ${LIBWEBSOCKETS_SOURCE_DIR} ...")

    execute_process(
      COMMAND "${GIT_EXECUTABLE}" clone --branch main --depth 1
              https://github.com/warmcat/libwebsockets.git "${LIBWEBSOCKETS_SOURCE_DIR}"
      RESULT_VARIABLE LIBWEBSOCKETS_CLONE_RESULT)

    if(NOT LIBWEBSOCKETS_CLONE_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to clone libwebsockets (git exit code ${LIBWEBSOCKETS_CLONE_RESULT})")
    endif()
  endif()

  set(LIBWEBSOCKETS_C_FLAGS "${CMAKE_C_FLAGS}")

  include("${CMAKE_SOURCE_DIR}/CMakeModules/BuildLibwebsockets.cmake")
  build_libwebsockets()

  # gamja (https://codeberg.org/emersion/gamja, AGPLv3) - tiny web IRC
  # client, served directly by lc_lws via a libwebsockets HTTP mount at "/".
  # Deliberately NOT an npm/yarn project as far as chaosircd's build is
  # concerned (see CLAUDE.md's "No npm/yarn Projects" rule): gamja's
  # index.html loads main.js as a plain native ES module - no bundler is
  # required to run it, only to produce an optional minified build, which we
  # don't do. Its only three dependencies are themselves pre-built,
  # single-file ES modules; lib/index.js imports them by their exact
  # unmodified relative "../node_modules/<pkg>/dist/..." paths, so rather
  # than run `npm install` to populate node_modules/, those three files are
  # fetched directly by pinned URL+hash below - vendoring the same bytes
  # npm would have, without ever invoking npm.
  set(GAMJA_SOURCE_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/gamja"
      CACHE PATH "gamja source directory")

  if(NOT EXISTS "${GAMJA_SOURCE_DIR}/index.html")
    find_package(Git REQUIRED)

    message(STATUS "Cloning gamja into ${GAMJA_SOURCE_DIR} ...")

    execute_process(
      COMMAND "${GIT_EXECUTABLE}" clone --depth 1
              https://codeberg.org/emersion/gamja.git "${GAMJA_SOURCE_DIR}"
      RESULT_VARIABLE GAMJA_CLONE_RESULT)

    if(NOT GAMJA_CLONE_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to clone gamja (git exit code ${GAMJA_CLONE_RESULT})")
    endif()
  endif()

  # Pinned to the exact versions in gamja's own package-lock.json.
  file(
    DOWNLOAD https://cdn.jsdelivr.net/npm/preact@10.29.8/dist/preact.module.js
    "${GAMJA_SOURCE_DIR}/node_modules/preact/dist/preact.module.js"
    EXPECTED_HASH SHA256=c30e721ebfdc6e2ad4c18c14d2dfb82667829c8aec27de1207774e3fc16858a8)
  file(
    DOWNLOAD https://cdn.jsdelivr.net/npm/htm@3.1.1/dist/htm.module.js
    "${GAMJA_SOURCE_DIR}/node_modules/htm/dist/htm.module.js"
    EXPECTED_HASH SHA256=ab33dd3f38059b9be4d5f5350128eefb2356639c4e0bbe9d9e8b3ba75847e9e4)
  file(
    DOWNLOAD https://cdn.jsdelivr.net/npm/linkifyjs@4.3.3/dist/linkify.mjs
    "${GAMJA_SOURCE_DIR}/node_modules/linkifyjs/dist/linkify.mjs"
    EXPECTED_HASH SHA256=e282e302f91aac5ca726962dfe324e45cc0ef28330f1b505b04b95765abb2eb8)

  install(DIRECTORY "${GAMJA_SOURCE_DIR}/" DESTINATION "${datadir}/gamja" PATTERN ".git" EXCLUDE)
endif(BUILD_LC_LWS)

add_subdirectory(src)

add_subdirectory(modules)

# ---------------------------------------------------------------------------
# test targets: `make servauth-test` / `make tls-test`
# ---------------------------------------------------------------------------
find_program(BASH_EXECUTABLE bash)

if(BASH_EXECUTABLE)
  # servauth-test: self-contained, spawns its own servauth instance.
  add_custom_target(
    servauth-test
    COMMAND ${CMAKE_COMMAND} -E env "SERVAUTH_BIN=$<TARGET_FILE:servauth>" ${BASH_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/tests/servauth/test_roundtrip.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/servauth"
    DEPENDS servauth
    USES_TERMINAL
    COMMENT "Running servauth dns forward/reverse + proxy scan round trip")

  # tls-test: exercises a *running* ircd's SSL listener over the network,
  # it does not start one itself - point it at an already-running instance
  # (e.g. via the runit service in etc/service/chaosircd) with:
  #   cmake -DIRCD_TEST_HOST=... -DIRCD_TEST_PORT=... .
  set(IRCD_TEST_HOST
      "127.0.0.1"
      CACHE STRING "Host to connect to for `make tls-test`")
  set(IRCD_TEST_PORT
      "6697"
      CACHE STRING "Port to connect to for `make tls-test`")

  add_custom_target(
    tls-test
    COMMAND ${BASH_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tests/ircd/test_ssl_listener.sh" "${IRCD_TEST_HOST}"
            "${IRCD_TEST_PORT}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/ircd"
    DEPENDS chaosircd
    USES_TERMINAL
    COMMENT "Testing chaosircd SSL listener at ${IRCD_TEST_HOST}:${IRCD_TEST_PORT} (must already be running)")
else()
  message(STATUS "bash not found - servauth-test/tls-test targets not available")
endif()

# ---------------------------------------------------------------------------
# CPack: run `cpack` inside a configured build directory (e.g.
# build/x86_64-linux-gnu, after `make install` would normally be run) to
# package that build as a .deb and/or a plain .tar.gz, without going through
# the debian/ dir's dpkg-buildpackage flow. `cpack -G DEB` / `cpack -G TGZ`
# picks one generator; plain `cpack` builds both.
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "${PACKAGENAME}")
set(CPACK_PACKAGE_VERSION "${PACKAGEVERSION}")
set(CPACK_PACKAGE_CONTACT "Roman Senn <roman.l.senn@gmail.com>")
set(CPACK_PACKAGE_VENDOR "Roman Senn")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "modular IRC server with reverse-DNS/proxy-scan auth helper")
set(CPACK_GENERATOR "TGZ;DEB")

# Distinguish musl builds (build/x86_64-linux-musl) from glibc ones in the
# output filename, so packaging both doesn't produce two files with the
# same name.
set(CPACK_SYSTEM_NAME "Linux-x86_64")
if(CMAKE_C_COMPILER MATCHES "musl")
  set(CPACK_SYSTEM_NAME "${CPACK_SYSTEM_NAME}-musl")
endif()
set(CPACK_PACKAGE_FILE_NAME "${PACKAGENAME}-${PACKAGEVERSION}-${CPACK_SYSTEM_NAME}")

# Both generators otherwise re-run the install step with their own default
# packaging root ("/usr" for DEB, "/" for TGZ), which only the install()
# calls using an already-absolute *dir cache var (sysconfdir, libexecdir,
# sbindir, ...) survive - anything installed with a plain relative
# DESTINATION (bin, lib, include/...) would silently move under /usr
# instead of staying under CMAKE_INSTALL_PREFIX. Pin it to match.
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Mirrors debian/control - see that file if these two diverge.
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Roman Senn <roman.l.senn@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "runit")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

include(CPack)
