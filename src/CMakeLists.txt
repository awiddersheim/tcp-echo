file(GLOB_RECURSE COMMON_SOURCES "*.c")
list(
  REMOVE_ITEM
  COMMON_SOURCES
  ${CMAKE_SOURCE_DIR}/src/controller.c
  ${CMAKE_SOURCE_DIR}/src/worker.c
)

configure_file(
  config.h.cmake
  ${CMAKE_SOURCE_DIR}/src/config.h
)

add_executable(
  tcp-echo-controller
  controller.c
  ${COMMON_SOURCES}
)

add_executable(
  tcp-echo-worker
  worker.c
  ${COMMON_SOURCES}
)

set_target_properties(
  tcp-echo-controller
  tcp-echo-worker
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_link_libraries(
  tcp-echo-controller
  PRIVATE
  ${LIBUV_STATIC_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT}
  SDS_STATIC_LIBRARY
)

target_link_libraries(
  tcp-echo-worker
  PRIVATE
  ${LIBUV_STATIC_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT}
  SDS_STATIC_LIBRARY
)
