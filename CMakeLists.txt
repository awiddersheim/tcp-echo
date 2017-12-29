cmake_minimum_required(VERSION 2.8)
include(CheckCSourceCompiles)

project(server)

CHECK_C_SOURCE_COMPILES("
  #include <string.h>
  int main() { char x = *strerror_r(0, &x, sizeof(x)); return 0; }
  " STRERROR_R_CHAR_P)

# Now create a useable config.h
configure_file(
  ${PROJECT_SOURCE_DIR}/config.h.cmake
  ${PROJECT_SOURCE_DIR}/config.h
)

add_library(log log.c)
add_library(utils utils.c)
find_package(Threads)
add_executable(server main.c)
target_link_libraries(server PRIVATE log)
target_link_libraries(server PRIVATE utils)
target_link_libraries(server PRIVATE ${CMAKE_THREAD_LIBS_INIT})
