include(CheckLibraryExists)

check_library_exists(dl dlsym "" HAVE_DLFCN)

if(HAVE_DLFCN)
  set(DLFCN_LIBRARIES dl)
endif()