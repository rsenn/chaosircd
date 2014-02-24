MACRO(substitute_project_includes NAME)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/${ARGV0}"
    "${CMAKE_CURRENT_BINARY_DIR}/include"
  )
  
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/${ARGV0}/config.h.cmake"
                 "${CMAKE_CURRENT_BINARY_DIR}/include/${ARGV0}/config.h")
                 
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake"
                 "${CMAKE_CURRENT_BINARY_DIR}/config.h")
  	       
ENDMACRO(substitute_project_includes)