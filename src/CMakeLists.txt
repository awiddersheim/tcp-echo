file(GLOB_RECURSE COMMON_SOURCES "*.c")
list(REMOVE_ITEM COMMON_SOURCES "master.c")

configure_file(
  config.h.cmake
  ${CMAKE_SOURCE_DIR}/src/config.h
)

add_executable(
  tcp-echo-master
  master.c
  ${COMMON_SOURCES}
)

set_target_properties(
  tcp-echo-master
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_link_libraries(
  tcp-echo-master
  PRIVATE
  ${LIBUV_STATIC_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT}
)
