include(FindFreetype)

if(FREETYPE_LIBRARY)
  set(USE_FREETYPE true CACHE BOOL "Use FreeType")
endif()
if(FREETYPE_FOUND)
  set(USE_FREETYPE true CACHE BOOL "Use FreeType")
endif()
if(USE_FREETYPE)
  set(FREETYPE_LIBS "${FREETYPE_LIBRARIES}")
  include_directories(${FREETYPE_INCLUDE_DIRS})
  add_definitions(-DHAVE_FREETYPE=1)
endif()

