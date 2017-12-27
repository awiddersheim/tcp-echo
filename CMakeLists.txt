cmake_minimum_required (VERSION 2.8)

project (server)

add_library (log log.c)
add_library (utils utils.c)
find_package (Threads)
add_executable (server main.c)
target_link_libraries (server PRIVATE log)
target_link_libraries (server PRIVATE utils)
target_link_libraries (server PRIVATE ${CMAKE_THREAD_LIBS_INIT})
