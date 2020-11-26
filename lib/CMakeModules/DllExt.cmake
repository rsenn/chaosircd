set(DLLEXT "${CMAKE_SHARED_MODULE_SUFFIX}")

if(NOT DLLEXT)
  if(LINUX)
    set(DLLEXT "so" CACHE "shared object extension")
  endif(LINUX)
  if(WINDOWS)
    set(DLLEXT "dll" CACHE "shared object extension")
  endif(WINDOWS)
  if(MAC)
    set(DLLEXT "dylib" CACHE "shared object extension")
  endif(MAC)
endif()

add_definitions(-DDLLEXT=\"${DLLEXT}\")
