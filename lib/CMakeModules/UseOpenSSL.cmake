include(FindOpenSSL)

if(OPENSSL_LIBRARY)
  set(USE_OPENSSL true CACHE BOOL "Use OpenSSL")
endif()
if(OPENSSL_FOUND)
  set(USE_OPENSSL true CACHE BOOL "Use OpenSSL")
endif()
if(USE_OPENSSL)
  set(OPENSSL_LIBS "${OPENSSL_LIBRARIES}")
  include_directories(${OPENSSL_INCLUDE_DIRS})
  add_definitions(-DHAVE_OPENSSL=1)
endif()

