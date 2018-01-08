cmake_minimum_required(VERSION 2.8)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)

project(tcp-echo)

CHECK_C_SOURCE_COMPILES("
  #include <string.h>
  int main() { char x = *strerror_r(0, &x, sizeof(x)); return 0; }
  " STRERROR_R_CHAR_P)

configure_file(
  ${PROJECT_SOURCE_DIR}/config.h.cmake
  ${PROJECT_SOURCE_DIR}/config.h
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|.*Clang")
  foreach (_CCOPT -Wall -Wextra)
    CHECK_C_COMPILER_FLAG(${_CCOPT} OPT${_CCOPT})
    if(OPT${_CCOPT})
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_CCOPT}")
    endif()
  endforeach()
endif()

find_package(Threads)

add_executable(
    tcp-echo
    main.c
    log.c
    utils.c
    error.c
)

target_link_libraries(tcp-echo PRIVATE ${CMAKE_THREAD_LIBS_INIT})
