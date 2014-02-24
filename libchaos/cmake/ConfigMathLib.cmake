include(CheckLibraryExists)

check_library_exists(m sin "" HAVE_MATH)

if(HAVE_MATH)
  set(MATH_LIBRARIES m)
endif()