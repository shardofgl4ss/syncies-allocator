cmake_minimum_required(VERSION 3.27)
project(arena_allocator C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
include_directories(src/include)

set(CMAKE_C_STANDARD 23)

add_subdirectory(sync_alloc)
add_subdirectory(alloc_tester)

set(DEBUG_COMPILE_FLAGS
	-DALLOC_DEBUG=1
	-Wall
	-Wextra
	-Werror
	-Wshadow
	-Wformat=2
	-Wconversion
	-Wundef
	-Wdouble-promotion
	-Wcast-align
	-Wstrict-overflow=5
	-Wmissing-prototypes
	-Wno-format-nonliteral
	-fno-common
	-g3
	-Og
	-std=c23
	-march=native
)
