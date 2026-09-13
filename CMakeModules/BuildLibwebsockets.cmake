# Adapted from ../../plot-cv/quickjs/qjs-lws/cmake/BuildLibwebsockets.cmake -
# trimmed to what lc_lws actually needs (no wolfssl/mbedtls/gnutls/http3
# branches, no plugin system, no qjs-lws's own patches/ step - chaosircd's
# copy of libwebsockets is a plain, unpatched clone of upstream). Kept
# close to the original in shape/naming so the two are easy to diff.
include(CheckLibraryExists)

macro(build_libwebsockets)
  if(ARGN)
    set(TARGET "${ARGN}")
  else(ARGN)
    set(TARGET libwebsockets)
  endif(ARGN)

  message("-- Building LIBWEBSOCKETS from source")

  if(NOT DEFINED LIBWEBSOCKETS_C_FLAGS)
    message(FATAL_ERROR "Please set LIBWEBSOCKETS_C_FLAGS before including this file.")
  endif()

  set(LWS_WITHOUT_TESTAPPS TRUE)
  set(LWS_WITHOUT_TEST_SERVER TRUE)
  set(LWS_WITHOUT_TEST_PING TRUE)
  set(LWS_WITHOUT_TEST_CLIENT TRUE)
  set(LWS_LINK_TESTAPPS_DYNAMIC OFF CACHE BOOL "link test apps dynamic")
  set(LWS_WITH_STATIC ON CACHE BOOL "build libwebsockets static library")
  set(LWS_HAVE_LIBCAP FALSE CACHE BOOL "have libcap")

  if(NOT "${LIBWEBSOCKETS_INCLUDE_DIR}")
    unset(LIBWEBSOCKETS_INCLUDE_DIR CACHE)
  endif(NOT "${LIBWEBSOCKETS_INCLUDE_DIR}")

  if(NOT EXISTS "${LIBWEBSOCKETS_INCLUDE_DIR}")
    set(LIBWEBSOCKETS_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/libwebsockets)
  endif(NOT EXISTS "${LIBWEBSOCKETS_INCLUDE_DIR}")

  include_directories(
    ${LIBWEBSOCKETS_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/libwebsockets
    ${CMAKE_CURRENT_BINARY_DIR}/libwebsockets/include)

  set(LIBWEBSOCKETS_FOUND ON CACHE BOOL "found libwebsockets")

  check_library_exists(cap cap_init "" LIBCAP)
  if(LIBCAP)
    set(LIBCAP_LIBRARY cap)
  endif(LIBCAP)

  set(LIBWEBSOCKETS_LIBRARIES "${LIBCAP_LIBRARY}")
  if(USE_OPENSSL AND OPENSSL_LIBRARIES)
    set(LIBWEBSOCKETS_LIBRARIES "${OPENSSL_LIBRARIES};${LIBWEBSOCKETS_LIBRARIES}")
  endif(USE_OPENSSL AND OPENSSL_LIBRARIES)

  # LWS_WITH_SSL tracks the outer project's own OpenSSL detection
  # (UseOpenSSL.cmake's USE_OPENSSL/HAVE_OPENSSL) rather than being forced
  # ON - e.g. under a musl cross-build with no musl-built OpenSSL on the
  # host, FindOpenSSL finds only the host's glibc headers (OPENSSL_LIBRARIES
  # stays NOTFOUND, so chaosircd itself already builds without SSL - see
  # HAVE_SSL). Forcing LWS_WITH_SSL=ON there used to make the *libwebsockets*
  # sub-build run its own find_package(OpenSSL), which (with no toolchain
  # file restricting find_* to a musl sysroot) also resolved to
  # /usr/include, and once forwarded that leaked host glibc headers into
  # every libwebsockets translation unit (not just the SSL-related ones),
  # breaking the musl compile on unrelated files.
  if(USE_OPENSSL)
    set(LIBWEBSOCKETS_WITH_SSL ON)
  else()
    set(LIBWEBSOCKETS_WITH_SSL OFF)
  endif()

  set(LIBWEBSOCKETS_ARGS
      -DLWS_WITH_SSL:BOOL=${LIBWEBSOCKETS_WITH_SSL}
      -DLWS_WITH_WOLFSSL:BOOL=OFF
      -DLWS_WITH_MBEDTLS:BOOL=OFF
      -DLWS_WITH_GNUTLS:BOOL=OFF)

  if(CMAKE_TOOLCHAIN_FILE)
    set(LIBWEBSOCKETS_ARGS ${LIBWEBSOCKETS_ARGS}
        -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE})
  endif(CMAKE_TOOLCHAIN_FILE)

  # Only forward OpenSSL's location when the outer project actually has a
  # usable (headers + libraries) OpenSSL - forwarding OPENSSL_INCLUDE_DIR
  # alone, as found by a bare header-only FindOpenSSL hit, is what leaked
  # glibc headers above.
  if(USE_OPENSSL AND OPENSSL_LIBRARIES)
    set(LIBWEBSOCKETS_ARGS "${LIBWEBSOCKETS_ARGS}"
        -DLWS_OPENSSL_LIBRARIES:STRING=${OPENSSL_LIBRARIES}
        -DOPENSSL_LIBRARIES:STRING=${OPENSSL_LIBRARIES})
  endif(USE_OPENSSL AND OPENSSL_LIBRARIES)
  if(USE_OPENSSL AND OPENSSL_INCLUDE_DIR)
    set(LIBWEBSOCKETS_ARGS "${LIBWEBSOCKETS_ARGS}"
        -DLWS_OPENSSL_INCLUDE_DIRS:STRING=${OPENSSL_INCLUDE_DIR}
        -DOPENSSL_INCLUDE_DIR:STRING=${OPENSSL_INCLUDE_DIR})
  endif(USE_OPENSSL AND OPENSSL_INCLUDE_DIR)

  # FORCE: build_libwebsockets() is a macro, so the plain set() calls above
  # leak LIBWEBSOCKETS_LIBRARIES into the caller's scope as an ordinary
  # variable (missing the "websockets" entry). Without FORCE, this CACHE
  # set() is a no-op on any reconfigure of an already-configured build dir
  # (the cache entry already exists) - the ordinary variable then keeps
  # shadowing it for the rest of that run, so ${LIBWEBSOCKETS_LIBRARIES}
  # silently loses "websockets" (i.e. -lwebsockets) and every reconfigure
  # after the first one links lc_lws.so with unresolved lws_* symbols.
  set(LIBWEBSOCKETS_LIBRARIES "websockets;${LIBWEBSOCKETS_LIBRARIES}"
      CACHE STRING "libwebsockets libraries" FORCE)
  set(LIBWEBSOCKETS_INCLUDE_DIR "${LIBWEBSOCKETS_INCLUDE_DIR}"
      CACHE PATH "libwebsockets include directory")
  set(LIBWEBSOCKETS_LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libwebsockets/lib
      CACHE PATH "libwebsockets library directory")

  list(APPEND LIBWEBSOCKETS_ARGS -DLWS_HAVE_HMAC_CTX_new:INTERNAL=1
       -DLWS_HAVE_RSA_SET0_KEY:INTERNAL=1 -DLWS_HAVE_ECDSA_SIG_set0:INTERNAL=1
       -DLWS_HAVE_BN_bn2binpad:INTERNAL=1)

  include(ExternalProject)

  ExternalProject_Add(
    "${TARGET}"
    SOURCE_DIR ${LIBWEBSOCKETS_SOURCE_DIR}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libwebsockets
    PREFIX libwebsockets
    CMAKE_ARGS
      "-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}"
      "-DCMAKE_C_FLAGS:STRING=${LIBWEBSOCKETS_C_FLAGS}"
      "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
      -DBUILD_TESTING:BOOL=OFF
      -DDISABLE_WERROR:BOOL=ON
      -DLWS_WITHOUT_TESTAPPS:BOOL=ON
      -DLWS_WITHOUT_TEST_SERVER:BOOL=ON
      -DLWS_WITHOUT_TEST_PING:BOOL=ON
      -DLWS_WITHOUT_TEST_CLIENT:BOOL=ON
      -DLWS_WITH_SHARED:BOOL=OFF
      -DLWS_WITH_STATIC:BOOL=ON
      -DLWS_STATIC_PIC:BOOL=ON
      -DLWS_IPV6:BOOL=ON
      -DLWS_ROLE_WS:BOOL=ON
      -DLWS_ROLE_RAW_PROXY:BOOL=OFF
      -DLWS_ROLE_MQTT:BOOL=OFF
      -DLWS_ROLE_DBUS:BOOL=OFF
      -DLWS_WITH_HTTP2:BOOL=OFF
      -DLWS_WITH_HTTP3:BOOL=OFF
      -DLWS_WITH_EXTERNAL_POLL:BOOL=ON
      -DLWS_WITHOUT_EXTENSIONS:BOOL=ON
      -DLWS_WITH_ZLIB:BOOL=OFF
      -DLWS_WITH_HTTP_BROTLI:BOOL=OFF
      -DLWS_WITH_PLUGINS:BOOL=OFF
      -DLWS_WITHOUT_DAEMONIZE:BOOL=ON
      -DLWS_WITH_NO_LOGS:BOOL=OFF
      -DLWS_SUPPRESS_DEPRECATED_API_WARNINGS:BOOL=ON
      -DLWS_HAVE_LIBCAP:BOOL=FALSE
    CMAKE_CACHE_ARGS ${LIBWEBSOCKETS_ARGS}
    INSTALL_COMMAND ""
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD ON)

  if(ARGN)
    ExternalProject_Add_StepDependencies("${TARGET}" build ${ARGN})
  endif(ARGN)
endmacro(build_libwebsockets)
