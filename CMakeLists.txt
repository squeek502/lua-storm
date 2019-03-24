cmake_minimum_required (VERSION 3.1)
project (luastorm C)

option(WITH_SHARED_STORMLIB "Link to a shared stormlib library instead of static linking" OFF)

if(UNIX)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(WIN32)
  find_package (Lua REQUIRED)
else()
  # Only the include dir matters on UNIX
  find_package (Lua)
  if(NOT LUA_INCLUDE_DIR)
    message( FATAL_ERROR "Variable `LUA_INCLUDE_DIR' expected to be defined.")
  endif()
endif()
include_directories(${LUA_INCLUDE_DIR})

# stormlib
if (WITH_SHARED_STORMLIB)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
  find_package(Stormlib REQUIRED)
else ()
  add_subdirectory(StormLib)
  set(STORMLIB_LIBRARIES storm)
endif ()

# lua-storm
add_library( luastorm MODULE storm.c )
target_link_libraries ( luastorm ${STORMLIB_LIBRARIES} )
target_include_directories ( luastorm PRIVATE ${STORMLIB_INCLUDE_DIR} )
set_target_properties( luastorm PROPERTIES PREFIX "" OUTPUT_NAME "storm" )
if(WIN32)
  target_compile_definitions(luastorm PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Only link against the Lua library on Windows
if(WIN32)
  target_link_libraries ( luastorm ${LUA_LIBRARIES} )
endif()

if (NOT WIN32)
  target_link_libraries ( luastorm stdc++ )
endif()

# Needed to avoid getting undefined references to liblua
if(APPLE)
  set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -undefined dynamic_lookup")
endif()

# Build static library when cross-compiling
if(WIN32 AND CMAKE_HOST_UNIX)
  add_library( libluastorm STATIC storm.c )
  target_link_libraries ( libluastorm ${LUA_LIBRARIES} )
  target_include_directories ( libluastorm PRIVATE ${STORMLIB_INCLUDE_DIR} )
  set_target_properties ( libluastorm PROPERTIES OUTPUT_NAME luastorm )
  if(WIN32)
    target_compile_definitions(libluastorm PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
endif()

# Install targets for Luarocks and non-Luarocks
if(LUAROCKS_INSTALL_LIBDIR)
  SET(INSTALL_LIB_DIR ${LUAROCKS_INSTALL_LIBDIR})
elseif(UNIX)
  set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}")
elseif(WIN32)
  set(INSTALL_LIB_DIR "${PROJECT_SOURCE_DIR}")
endif()

if(INSTALL_LIB_DIR)
  install(TARGETS luastorm ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" LIBRARY DESTINATION "${INSTALL_LIB_DIR}")
endif()