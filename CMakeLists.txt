cmake_minimum_required(VERSION 3.27)
project(arena_allocator C)

include_directories(include)
include_directories(src/include)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS "-D ALLOC_DEBUG=1 -Wall -Wextra -Werror -Wshadow -Wformat=2 -Wconversion -fno-common -Wundef -Wdouble-promotion -Wno-format-nonliteral -g3 -O0")
set(SYNC_ALLOCATOR sync_alloc)
set(ALLOC_TESTER alloc_tester)

add_subdirectory(sync_alloc)
add_subdirectory(alloc_tester)
