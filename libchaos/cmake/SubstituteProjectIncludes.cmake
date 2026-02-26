# substitute_project_includes(NAME SOURCE_DIR BINARY_DIR)
macro(substitute_project_includes NAME)

  include_directories(
    "${ARGV1}/src" "${ARGV1}/include"
    # "${ARGV1}/include/${ARGV0}"
    "${ARGV2}/include")

  configure_file("${ARGV1}/include/${ARGV0}/config.h.cmake" "${ARGV2}/include/${ARGV0}/config.h")

  configure_file("${ARGV1}/config.h.cmake" "${ARGV2}/config.h")

endmacro(substitute_project_includes)

# include_project_includes(NAME SOURCE_DIR BINARY_DIR)
macro(include_project_includes NAME)

  include_directories(
    "${ARGV1}/src" "${ARGV1}/include"
    # "${ARGV1}/include/${ARGV0}"
    "${ARGV2}/include")
endmacro(include_project_includes)
