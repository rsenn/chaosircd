include(CheckFunctionExists)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
  include(ConfigStdIncludes)
  include(ConfigSockets)
  include(ConfigDlFcn)
  include(ConfigIntTypes)


if(NOT HAVE_DLFCN)
  if(CMAKE_HOST_WIN32)
    set(DLFCN_SOURCES src/dlfcn_win32.c)
  endif(CMAKE_HOST_WIN32)  
endif(NOT HAVE_DLFCN)


if(CMAKE_HOST_WIN32)
  if(NOT HAVE_SELECT)
    if(HAVE_WINSOCK2_H)
      set(SOCKET_LIBRARIES ws2_32)
    else(HAVE_WINSOCK2_H)
      set(SOCKET_LIBRARIES wsock32)
    endif(HAVE_WINSOCK2_H)
  endif(NOT HAVE_SELECT)
  set(USE_SELECT TRUE)
else(CMAKE_HOST_WIN32)

  check_function_exists(poll HAVE_POLL)

  if(HAVE_POLL)
    set(USE_POLL TRUE)
  elseif(HAVE_SELECT)
    set(USE_SELECT TRUE)
  endif()

endif(CMAKE_HOST_WIN32)


if(HAVE_POLL)
  set(USE_POLL 1)
else(HAVE_POLL)
  set(USE_SELECT 1)
endif(HAVE_POLL)
