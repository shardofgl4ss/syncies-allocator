cmake_minimum_required(VERSION 3.27)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS "-Wno-ignored-attributes")

project(arena_allocator C)

add_executable(tester)

add_library(syn_memops_test STATIC)
add_library(sync_alloc STATIC)

add_subdirectory(sync_alloc)
add_subdirectory(alloc_tester)

set_target_properties(sync_alloc PROPERTIES C_VISIBILITY_PRESET hidden)

target_link_libraries(tester PUBLIC sync_alloc)
