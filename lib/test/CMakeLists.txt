set(LIBCHAOS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(LIBCHAOS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/..")
set(CMAKE_MODULE_PATH "${LIBCHAOS_SOURCE_DIR}/cmake")

include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/PlatformChecks.cmake")

if(NOT DEFINED BUILD_SHARED)
  set(BUILD_SHARED ${DEFAULT_SHARED_LIB})
endif(NOT DEFINED BUILD_SHARED)

if(NOT BUILD_SHARED)
  add_definitions(-DSTATIC_LIBCHAOS=1)
endif(NOT BUILD_SHARED)

include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/SubstituteProjectIncludes.cmake")

include_project_includes(libchaos "${LIBCHAOS_SOURCE_DIR}"
                         "${LIBCHAOS_BINARY_DIR}")

link_directories("${LIBCHAOS_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/../..")

set(TESTS cfgtest dbtest giftest graphtest imagetest ttftest)

foreach(TEST ${TESTS})

  add_executable("${TEST}" "${TEST}.c")
  target_link_libraries("${TEST}" chaos owfat ${MYSQL_LIBS} ${OPENSSL_LIBS})

endforeach()
