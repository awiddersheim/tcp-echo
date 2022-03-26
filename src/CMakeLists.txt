file(GLOB_RECURSE SOURCES "*.c")

configure_file(
  constants.h.cmake
  ${CMAKE_SOURCE_DIR}/src/constants.h
)

add_executable(
  tcp-echo
  ${SOURCES}
)

set_target_properties(
  tcp-echo
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_link_libraries(
  tcp-echo
  PRIVATE
  ${LIBUV_STATIC_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT}
  SDS_STATIC_LIBRARY
)
