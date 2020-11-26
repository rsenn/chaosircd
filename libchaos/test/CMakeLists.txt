set(LIBCHAOS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(LIBCHAOS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/..")
set(CMAKE_MODULE_PATH "${LIBCHAOS_SOURCE_DIR}/cmake")

include("${LIBCHAOS_SOURCE_DIR}/cmake/PlatformChecks.cmake")

if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ${DEFAULT_SHARED_LIB})
endif(NOT DEFINED BUILD_SHARED_LIBS)

if(NOT BUILD_SHARED_LIBS)
  # add_definitions(-DSTATIC_LIBCHAOS=1)
endif(NOT BUILD_SHARED_LIBS)

include("${LIBCHAOS_SOURCE_DIR}/cmake/SubstituteProjectIncludes.cmake")

include_project_includes(libchaos "${LIBCHAOS_SOURCE_DIR}"
                         "${LIBCHAOS_BINARY_DIR}")

link_directories("${LIBCHAOS_BINARY_DIR}")

add_executable(cfgtest cfgtest.c)
target_link_libraries(cfgtest libchaos)

# add_executable(dbtest dbtest.c) target_link_libraries(dbtest libchaos)

add_executable(giftest giftest.c)
target_link_libraries(giftest libchaos)

add_executable(graphtest graphtest.c)
target_link_libraries(graphtest libchaos)

add_executable(imagetest imagetest.c)
target_link_libraries(imagetest libchaos)

add_executable(ttftest ttftest.c)
target_link_libraries(ttftest libchaos)
