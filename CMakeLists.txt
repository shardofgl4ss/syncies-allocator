cmake_minimum_required(VERSION 3.27)
project(arena_allocator C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS "-fvisibility=hidden -g3 -Wall -Wextra -Wshadow -Wformat=2 -Wconversion -fno-common -Wundef -Wdouble-promotion -Werror")
set(SYNC_ALLOCATOR sync_alloc)
set(ALLOC_TESTER alloc_tester)

add_subdirectory(sync_alloc)
add_subdirectory(alloc_tester)