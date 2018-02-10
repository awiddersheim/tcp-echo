cmake_minimum_required(VERSION 2.8)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)

project(tcp-echo)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

CHECK_C_SOURCE_COMPILES("
  #include <string.h>
  int main() { char x = *strerror_r(0, &x, sizeof(x)); return 0; }
  " STRERROR_R_CHAR_P)

configure_file(
  ${PROJECT_SOURCE_DIR}/src/config.h.cmake
  ${PROJECT_SOURCE_DIR}/src/config.h
)

set(CMAKE_C_FLAGS "-std=gnu89")

if(CMAKE_C_COMPILER_ID MATCHES "GNU|.*Clang")
  foreach (_CCOPT -Wall -Wextra -Wunused -Wunreachable-code -Wpedantic)
    CHECK_C_COMPILER_FLAG(${_CCOPT} OPT${_CCOPT})
    if(OPT${_CCOPT})
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_CCOPT}")
    endif()
  endforeach()
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(LIBUV_DEPS_DIR ${CMAKE_SOURCE_DIR}/deps/libuv)
configure_file(cmake/in/libuv.in ${LIBUV_DEPS_DIR}/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${LIBUV_DEPS_DIR})
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${LIBUV_DEPS_DIR})
include_directories(${LIBUV_DEPS_DIR}/src/include)
find_library(LIBUV_STATIC_LIBRARY NAMES libuv.a libuv PATHS ${LIBUV_DEPS_DIR}/src PATH_SUFFIXES .libs Release NO_DEFAULT_PATH)

add_subdirectory(src)
