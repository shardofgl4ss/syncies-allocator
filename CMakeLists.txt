cmake_minimum_required(VERSION 3.27)
project(arena_allocator C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wshadow -Wformat=2 -Wconversion -fno-common -Wundef -Wdouble-promotion -Werror")

add_subdirectory(sync_alloc)

add_executable(main main.c
			   sync_alloc/include/sync_alloc.h)
target_link_libraries(sync_alloc)