if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -O0 -Wall")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -ggdb -O2 -Wall")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  set(CMAKE_C_FLAGS_MINSIZEREL "-fomit-frame-pointer -Wall -Os")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
   set(CMAKE_C_FLAGS_RELEASE "-g -Wall -O2")
endif()


