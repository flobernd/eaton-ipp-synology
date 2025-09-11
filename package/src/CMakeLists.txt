cmake_minimum_required(VERSION 3.15...4.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options("-Wall" "-Wextra" "-Werror" "-Wpedantic" "$<$<CONFIG:RELEASE>:-O3>")

project(
  ipp_nut_proxy
  VERSION 1.0
  LANGUAGES C
)

find_package(Threads REQUIRED)

add_library(kv_store
    kv_store.c
    kv_store.h
)

target_link_libraries(kv_store PRIVATE Threads::Threads)

add_library(nut_server
    nut_handler.c
    nut_proto.c
    nut_server.c
    nut_client_context.h
    nut_handler.h
    nut_proto.h
    nut_server.h
)

target_link_libraries(nut_server PRIVATE kv_store)

add_executable(
    ipp_nut_proxy
    ipp_nut_proxy.c
    watcher.c
    watcher.h
)

target_link_libraries(ipp_nut_proxy PRIVATE kv_store nut_server)
