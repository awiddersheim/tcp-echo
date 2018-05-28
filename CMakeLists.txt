cmake_minimum_required(VERSION 2.8)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)

project(tcp-echo C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(src)

set(LIBUV_DEPS_DIR ${CMAKE_BINARY_DIR}/deps/libuv)
configure_file(cmake/in/libuv.in ${LIBUV_DEPS_DIR}/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${LIBUV_DEPS_DIR})
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${LIBUV_DEPS_DIR})
include_directories(${LIBUV_DEPS_DIR}/src/include)
find_library(LIBUV_STATIC_LIBRARY NAMES libuv.a libuv PATHS ${LIBUV_DEPS_DIR}/src PATH_SUFFIXES .libs Release NO_DEFAULT_PATH)

set(SDS_DEPS_DIR ${CMAKE_BINARY_DIR}/deps/sds)
configure_file(cmake/in/sds.in ${SDS_DEPS_DIR}/CMakeLists.txt)
configure_file(cmake/in/sdsalloc.h.in ${SDS_DEPS_DIR}/src/sdsalloc.h)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${SDS_DEPS_DIR})
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${SDS_DEPS_DIR})
include_directories(${SDS_DEPS_DIR}/src)
add_library(SDS_STATIC_LIBRARY STATIC ${SDS_DEPS_DIR}/src/sds.c)

CHECK_C_SOURCE_COMPILES(
  "#include <string.h>
  int main() { char x = *strerror_r(0, &x, sizeof(x)); return 0; }"
  STRERROR_R_CHAR_P
)

set(CMAKE_C_FLAGS "-std=gnu99")

list(
  APPEND
  COMPILER_FLAGS
  -Wall
  -Wextra
  -Wunused
  -Wunreachable-code
  -Wpedantic
)

if(DEFINED ENV{CI})
  set(CMAKE_BUILD_TYPE Debug)

  list(
    APPEND
    COMPILER_FLAGS
    -Werror
    -pedantic-errors
  )
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|.*Clang")
  foreach (COMPILER_FLAG IN LISTS COMPILER_FLAGS)
    string(REPLACE "-" "" CLEAN_COMPILER_FLAG ${COMPILER_FLAG})

    CHECK_C_COMPILER_FLAG(${COMPILER_FLAG} FLAG_${CLEAN_COMPILER_FLAG})

    if(FLAG_${CLEAN_COMPILER_FLAG})
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_FLAG}")
    endif()
  endforeach()
endif()

add_subdirectory(src)
