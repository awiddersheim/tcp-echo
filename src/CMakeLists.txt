file(GLOB_RECURSE TCP_ECHO_SOURCES "*.c")

add_executable(tcp-echo ${TCP_ECHO_SOURCES})

set_target_properties(
  tcp-echo
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_link_libraries(tcp-echo PRIVATE ${LIBUV_STATIC_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
