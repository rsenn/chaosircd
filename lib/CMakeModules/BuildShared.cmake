if(NOT DEFINED DEFAULT_SHARED_LIB)
  if(WINDOWS OR CYGWIN)
    set(DEFAULT_SHARED_LIB TRUE)
  else()
    set(DEFAULT_SHARED_LIB FALSE)
  endif()
endif()

if(NOT DEFINED BUILD_SHARED)
  set(BUILD_SHARED "${DEFAULT_SHARED_LIB}" CACHE BOOL "Build dynamically linked binaries")
endif(NOT DEFINED BUILD_SHARED)

if(NOT BUILD_SHARED)
  set(BUILD_STATIC true CACHE BOOL "Build statically linked binaries")

else()
  set(BUILD_STATIC false CACHE BOOL "Build statically linked binaries")
endif(NOT BUILD_SHARED)

